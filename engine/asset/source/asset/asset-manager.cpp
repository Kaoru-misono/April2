#include "asset-manager.hpp"

#include "ddc/ddc-utils.hpp"
#include "importer/texture-importer.hpp"
#include "importer/gltf-importer.hpp"
#include "importer/material-importer.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <system_error>
#include <utility>

namespace april::asset
{
    AssetManager::AssetManager(std::filesystem::path const& assetRoot,
                               std::filesystem::path const& cacheRoot)
        : m_assetRoot{assetRoot}
        , m_ddc{cacheRoot}
    {
        AP_INFO("[AssetManager] Initialized. Asset root: {}, Cache root: {}",
                assetRoot.string(), cacheRoot.string());

        m_importers.registerImporter(std::make_unique<TextureImporter>());
        m_importers.registerImporter(std::make_unique<GltfImporter>());
        m_importers.registerImporter(std::make_unique<MaterialImporter>());
    }

    AssetManager::~AssetManager()
    {
        AP_INFO("[AssetManager] Shutdown.");
    }

    auto AssetManager::importAsset(std::filesystem::path const& sourcePath, ImportPolicy policy) -> std::shared_ptr<Asset>
    {
        if (!std::filesystem::exists(sourcePath))
        {
            AP_ERROR("[AssetManager] Import failed: Source file not found: {}", sourcePath.string());
            return nullptr;
        }

        auto extension = sourcePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });

        // Route GLTF/GLB files to dedicated importer
        if (extension == ".gltf" || extension == ".glb")
        {
            return importGltfFile(sourcePath, policy);
        }

        std::shared_ptr<Asset> existingAsset = nullptr;
        std::shared_ptr<Asset> newAsset = nullptr;

        auto assetFilePath = sourcePath;
        assetFilePath += ".asset";

        if (std::filesystem::exists(assetFilePath))
        {
            if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".tga")
            {
                existingAsset = loadAssetFromFile<TextureAsset>(assetFilePath);
            }
            else
            {
                AP_WARN("[AssetManager] Import skipped: Unsupported file extension '{}' for {}", extension, sourcePath.string());
                return nullptr;
            }

            if (policy == ImportPolicy::ReuseIfExists && existingAsset)
            {
                return existingAsset;
            }

            if (policy == ImportPolicy::ReimportIfSourceChanged)
            {
                auto targetId = m_targetProfile.toId();
                if (existingAsset)
                {
                    auto recordOpt = m_registry.findRecord(existingAsset->getHandle());
                    if (recordOpt.has_value())
                    {
                        auto currentHash = hashFileContents(sourcePath.string());
                        auto existingHashIt = recordOpt->lastSourceHash.find(targetId);
                        if (existingHashIt != recordOpt->lastSourceHash.end() && existingHashIt->second == currentHash)
                        {
                            return existingAsset;
                        }
                    }
                }
            }
        }

        if (existingAsset)
        {
            newAsset = existingAsset;
        }
        else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".tga")
        {
            newAsset = std::make_shared<TextureAsset>();
        }
        else
        {
            AP_WARN("[AssetManager] Import skipped: Unsupported file extension '{}' for {}", extension, sourcePath.string());
            return nullptr;
        }

        newAsset->setSourcePath(sourcePath.string());
        newAsset->setAssetPath(assetFilePath.string());

        if (auto importer = m_importers.findImporter(newAsset->getType()))
        {
            newAsset->setImporter(std::string{importer->id()}, importer->version());
        }

        nlohmann::json json;
        newAsset->serializeJson(json);

        auto file = std::ofstream{assetFilePath};
        if (!file.is_open())
        {
            AP_ERROR("[AssetManager] Failed to write .asset file: {}", assetFilePath.string());
            return nullptr;
        }
        file << json.dump(4);
        file.close();

        registerAssetInternal(newAsset, assetFilePath, true);

        AP_INFO("[AssetManager] Imported asset: {} -> {} (UUID: {})",
            sourcePath.string(), assetFilePath.string(), newAsset->getHandle().toString()
        );

        return newAsset;
    }

    auto AssetManager::importGltfFile(
        std::filesystem::path const& sourcePath,
        ImportPolicy policy
    ) -> std::shared_ptr<Asset>
    {
        auto assetFilePath = sourcePath;
        assetFilePath += ".asset";

        std::shared_ptr<StaticMeshAsset> existingAsset = nullptr;

        if (std::filesystem::exists(assetFilePath))
        {
            existingAsset = loadAssetFromFile<StaticMeshAsset>(assetFilePath);

            if (policy == ImportPolicy::ReuseIfExists && existingAsset)
            {
                return existingAsset;
            }

            if (policy == ImportPolicy::ReimportIfSourceChanged && existingAsset)
            {
                auto targetId = m_targetProfile.toId();
                auto recordOpt = m_registry.findRecord(existingAsset->getHandle());
                if (recordOpt.has_value())
                {
                    auto currentHash = hashFileContents(sourcePath.string());
                    auto existingHashIt = recordOpt->lastSourceHash.find(targetId);
                    if (existingHashIt != recordOpt->lastSourceHash.end() && existingHashIt->second == currentHash)
                    {
                        return existingAsset;
                    }
                }
            }
        }

        auto meshAsset = existingAsset ? existingAsset : std::make_shared<StaticMeshAsset>();
        meshAsset->setSourcePath(sourcePath.string());
        meshAsset->setAssetPath(assetFilePath.string());

        auto* importer = m_importers.findImporter(AssetType::Mesh);
        if (!importer)
        {
            AP_ERROR("[AssetManager] No importer found for Mesh type");
            return nullptr;
        }
        meshAsset->setImporter(std::string{importer->id()}, importer->version());

        // Setup ImportContext with callbacks for sub-asset handling
        auto deps = DepRecorder{};
        auto forceReimport = m_dirtyAssets.contains(meshAsset->getHandle()) || policy == ImportPolicy::Reimport;
        auto context = ImportContext{
            *meshAsset,
            meshAsset->getAssetPath(),
            meshAsset->getSourcePath(),
            m_targetProfile,
            m_ddc,
            deps,
            forceReimport
        };

        // Callback for finding existing assets by source path
        context.findAssetBySource = [this](std::filesystem::path const& path, AssetType type) -> std::shared_ptr<Asset>
        {
            return findAssetBySourcePath(path, type);
        };

        // Callback for registering sub-assets
        context.registerSubAsset = [this](std::shared_ptr<Asset> const& asset) -> void
        {
            if (asset)
            {
                registerAssetInternal(asset, asset->getAssetPath(), true);
            }
        };

        // Callback for importing sub-assets (textures)
        context.importSubAsset = [this](std::filesystem::path const& path) -> std::shared_ptr<Asset>
        {
            return importAsset(path, ImportPolicy::ReuseIfExists);
        };

        // Run the importer
        auto result = importer->import(context);

        if (!result.errors.empty())
        {
            for (auto const& error : result.errors)
            {
                AP_ERROR("[AssetManager] GLTF import error: {}", error);
            }
            return nullptr;
        }

        // Save mesh asset file
        auto json = nlohmann::json{};
        meshAsset->serializeJson(json);

        auto file = std::ofstream{assetFilePath};
        if (!file.is_open())
        {
            AP_ERROR("[AssetManager] Failed to write .asset file: {}", assetFilePath.string());
            return nullptr;
        }
        file << json.dump(4);
        file.close();

        registerAssetInternal(meshAsset, assetFilePath, true);

        // Update registry
        auto recordOpt = m_registry.findRecord(meshAsset->getHandle());
        auto record = recordOpt.value_or(AssetRecord{});
        record.guid = meshAsset->getHandle();
        record.assetPath = meshAsset->getAssetPath();
        record.type = AssetType::Mesh;
        record.deps = deps.deps;
        record.lastImportFailed = false;
        record.lastErrorSummary.clear();

        auto targetId = m_targetProfile.toId();
        record.ddcKeys[targetId] = result.producedKeys;
        if (!result.producedKeys.empty())
        {
            record.lastFingerprint[targetId] = result.producedKeys.front();
        }
        record.lastSourceHash[targetId] = hashFileContents(sourcePath.string());

        m_registry.updateRecord(std::move(record));

        if (forceReimport)
        {
            auto lock = std::scoped_lock{m_mutex};
            m_dirtyAssets.erase(meshAsset->getHandle());
        }

        AP_INFO("[AssetManager] Imported GLTF: {} -> {} (UUID: {})",
            sourcePath.string(), assetFilePath.string(), meshAsset->getHandle().toString()
        );

        return meshAsset;
    }

    auto AssetManager::findAssetBySourcePath(
        std::filesystem::path const& sourcePath,
        AssetType type
    ) -> std::shared_ptr<Asset>
    {
        auto assetPathFromIndex = std::optional<std::filesystem::path>{};
        auto handleFromIndex = std::optional<core::UUID>{};
        auto normalizedPath = normalizePath(sourcePath);

        {
            auto const key = buildSourceKey(type, sourcePath);
            auto lock = std::scoped_lock{m_mutex};
            if (auto it = m_sourcePathIndex.find(key); it != m_sourcePathIndex.end())
            {
                auto const handle = it->second;
                handleFromIndex = handle;
                if (m_loadedAssets.contains(handle))
                {
                    return m_loadedAssets.at(handle);
                }
            }

            for (auto const& [uuid, asset] : m_loadedAssets)
            {
                if (asset->getType() != type)
                {
                    continue;
                }

                auto assetSourcePath = std::filesystem::path{asset->getSourcePath()};
                if (!asset->getSourcePath().empty() && normalizePath(assetSourcePath) == normalizedPath)
                {
                    return asset;
                }

                if (type == AssetType::Material || type == AssetType::Texture)
                {
                    auto assetPath = std::filesystem::path{asset->getAssetPath()};
                    if (!asset->getAssetPath().empty() && normalizePath(assetPath) == normalizedPath)
                    {
                        return asset;
                    }
                }
            }
        }

        if (!assetPathFromIndex && handleFromIndex)
        {
            if (auto recordOpt = m_registry.findRecord(*handleFromIndex); recordOpt.has_value())
            {
                assetPathFromIndex = recordOpt->assetPath;
            }
        }

        if (assetPathFromIndex)
        {
            if (type == AssetType::Texture)
            {
                return loadAssetFromFile<TextureAsset>(*assetPathFromIndex);
            }
            if (type == AssetType::Material)
            {
                return loadAssetFromFile<MaterialAsset>(*assetPathFromIndex);
            }
            if (type == AssetType::Mesh)
            {
                return loadAssetFromFile<StaticMeshAsset>(*assetPathFromIndex);
            }
        }

        // Check if asset file exists and load it
        auto assetFilePath = sourcePath;
        if (type == AssetType::Texture)
        {
            assetFilePath += ".asset";
        }

        if (std::filesystem::exists(assetFilePath))
        {
            if (type == AssetType::Texture)
            {
                return loadAssetFromFile<TextureAsset>(assetFilePath);
            }
            else if (type == AssetType::Material)
            {
                return loadAssetFromFile<MaterialAsset>(assetFilePath);
            }
            else if (type == AssetType::Mesh)
            {
                return loadAssetFromFile<StaticMeshAsset>(assetFilePath);
            }
        }

        return nullptr;
    }

    auto AssetManager::getTextureData(TextureAsset const& asset, std::vector<std::byte>& outBlob) -> TexturePayload
    {
        auto key = ensureImported(asset);
        if (!key)
        {
            AP_ERROR("[AssetManager] Texture import failed: {}", asset.getSourcePath());
            return {};
        }

        auto value = DdcValue{};
        if (!m_ddc.get(*key, value))
        {
            AP_ERROR("[AssetManager] Missing texture DDC data for: {}", asset.getSourcePath());
            return {};
        }

        outBlob = std::move(value.bytes);

        if (outBlob.size() < sizeof(TextureHeader))
        {
            AP_ERROR("[AssetManager] Invalid texture blob size for: {}", asset.getSourcePath());
            return {};
        }

        auto payload = TexturePayload{};

        std::memcpy(&payload.header, outBlob.data(), sizeof(TextureHeader));

        if (!payload.header.isValid())
        {
            AP_ERROR("[AssetManager] Invalid texture header for: {}", asset.getSourcePath());
            return {};
        }

        auto const pixelDataOffset = sizeof(TextureHeader);
        auto const pixelDataSize = outBlob.size() - pixelDataOffset;

        if (pixelDataSize != payload.header.dataSize)
        {
            AP_WARN("[AssetManager] Texture data size mismatch: expected {}, got {}",
                    payload.header.dataSize, pixelDataSize);
        }

        payload.pixelData = std::span<std::byte const>{
            outBlob.data() + pixelDataOffset,
            static_cast<size_t>(payload.header.dataSize)
        };

        return payload;
    }

    auto AssetManager::getMeshData(StaticMeshAsset const& asset, std::vector<std::byte>& outBlob) -> MeshPayload
    {
        auto key = ensureImported(asset);
        if (!key)
        {
            AP_ERROR("[AssetManager] Mesh import failed: {}", asset.getSourcePath());
            return {};
        }

        auto value = DdcValue{};
        if (!m_ddc.get(*key, value))
        {
            AP_ERROR("[AssetManager] Missing mesh DDC data for: {}", asset.getSourcePath());
            return {};
        }

        outBlob = std::move(value.bytes);

        if (outBlob.size() < sizeof(MeshHeader))
        {
            AP_ERROR("[AssetManager] Invalid mesh blob size for: {}", asset.getSourcePath());
            return {};
        }

        auto payload = MeshPayload{};

        std::memcpy(&payload.header, outBlob.data(), sizeof(MeshHeader));

        if (!payload.header.isValid())
        {
            AP_ERROR("[AssetManager] Invalid mesh header for: {}", asset.getSourcePath());
            return {};
        }

        auto offset = sizeof(MeshHeader);

        auto const submeshDataSize = payload.header.submeshCount * sizeof(Submesh);
        if (offset + submeshDataSize > outBlob.size())
        {
            AP_ERROR("[AssetManager] Invalid mesh submesh data for: {}", asset.getSourcePath());
            return {};
        }

        payload.submeshes = std::span<Submesh const>{
            reinterpret_cast<Submesh const*>(outBlob.data() + offset),
            payload.header.submeshCount
        };
        offset += submeshDataSize;

        if (offset + payload.header.vertexDataSize > outBlob.size())
        {
            AP_ERROR("[AssetManager] Invalid mesh vertex data for: {}", asset.getSourcePath());
            return {};
        }

        payload.vertexData = std::span<std::byte const>{
            outBlob.data() + offset,
            static_cast<size_t>(payload.header.vertexDataSize)
        };
        offset += payload.header.vertexDataSize;

        if (offset + payload.header.indexDataSize > outBlob.size())
        {
            AP_ERROR("[AssetManager] Invalid mesh index data for: {}", asset.getSourcePath());
            return {};
        }

        payload.indexData = std::span<std::byte const>{
            outBlob.data() + offset,
            static_cast<size_t>(payload.header.indexDataSize)
        };

        return payload;
    }

    auto AssetManager::saveMaterialAsset(std::shared_ptr<MaterialAsset> const& material, std::filesystem::path const& outputPath) -> bool
    {
        if (!material)
        {
            AP_ERROR("[AssetManager] Cannot save null material asset");
            return false;
        }

        material->setAssetPath(outputPath.string());

        nlohmann::json json;
        material->serializeJson(json);

        auto file = std::ofstream{outputPath};
        if (!file.is_open())
        {
            AP_ERROR("[AssetManager] Failed to write material asset file: {}", outputPath.string());
            return false;
        }

        file << json.dump(4);
        file.close();

        registerAssetInternal(material, outputPath, true);

        AP_INFO("[AssetManager] Saved material asset: {} (UUID: {})",
            outputPath.string(), material->getHandle().toString());
        return true;
    }

    auto AssetManager::registerAssetPath(core::UUID handle, std::filesystem::path const& path) -> void
    {
        auto asset = loadAssetMetadata(path);
        if (asset)
        {
            if (asset->getHandle() != handle)
            {
                AP_WARN("[AssetManager] Asset UUID mismatch for {}: expected {}, got {}",
                    path.string(), handle.toString(), asset->getHandle().toString());
            }
            registerAssetInternal(asset, path, false);
        }

        AP_INFO("[AssetManager] Registered asset: {} -> {}", handle.toString(), path.string());
    }

    auto AssetManager::scanDirectory(std::filesystem::path const& directory) -> size_t
    {
        auto count = size_t{0};

        if (!std::filesystem::exists(directory))
        {
            AP_WARN("[AssetManager] Directory does not exist: {}", directory.string());
            return count;
        }

        for (auto const& entry : std::filesystem::recursive_directory_iterator(directory))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".asset")
            {
                auto asset = loadAssetMetadata(entry.path());
                if (asset)
                {
                    registerAssetInternal(asset, entry.path(), false);
                    ++count;
                }
            }
        }

        AP_INFO("[AssetManager] Scanned directory '{}': found {} assets", directory.string(), count);
        return count;
    }

    auto AssetManager::getDdc() -> LocalDdc&
    {
        return m_ddc;
    }

    auto AssetManager::registerAssetInternal(
        std::shared_ptr<Asset> const& asset,
        std::filesystem::path const& assetPath,
        bool cacheAsset
    ) -> void
    {
        if (!asset)
        {
            return;
        }

        auto const handle = asset->getHandle();
        {
            auto lock = std::scoped_lock{m_mutex};
            if (cacheAsset)
            {
                m_loadedAssets[handle] = asset;
            }
            if (!asset->getSourcePath().empty())
            {
                m_sourcePathIndex[buildSourceKey(asset->getType(), asset->getSourcePath())] = handle;
            }
            if (!asset->getAssetPath().empty() &&
                (asset->getType() == AssetType::Material || asset->getType() == AssetType::Texture))
            {
                m_sourcePathIndex[buildSourceKey(asset->getType(), asset->getAssetPath())] = handle;
            }
        }
        m_registry.registerAsset(*asset, assetPath.string());
    }

    auto AssetManager::markDependentsDirty(core::UUID const& guid) -> void
    {
        auto dependents = m_registry.getDependents(guid);
        {
            auto lock = std::scoped_lock{m_mutex};
            for (auto const& dependent : dependents)
            {
                m_dirtyAssets.insert(dependent);
            }
        }
    }

    auto AssetManager::loadAssetMetadata(std::filesystem::path const& assetPath) -> std::shared_ptr<Asset>
    {
        auto file = std::ifstream{assetPath};
        if (!file.is_open())
        {
            AP_ERROR("[AssetManager] Failed to open asset file: {}", assetPath.string());
            return nullptr;
        }

        auto json = nlohmann::json{};
        try
        {
            file >> json;
        }
        catch (nlohmann::json::parse_error const& e)
        {
            AP_ERROR("[AssetManager] Failed to parse asset file: {} - {}", assetPath.string(), e.what());
            return nullptr;
        }

        if (!json.contains("type"))
        {
            AP_ERROR("[AssetManager] Asset file missing 'type' field: {}", assetPath.string());
            return nullptr;
        }

        auto typeStr = json["type"].get<std::string>();
        auto type = AssetType::None;

        if (typeStr == "Texture")
        {
            type = AssetType::Texture;
        }
        else if (typeStr == "Mesh")
        {
            type = AssetType::Mesh;
        }
        else if (typeStr == "Material")
        {
            type = AssetType::Material;
        }

        if (type == AssetType::None)
        {
            AP_ERROR("[AssetManager] Unknown asset type: {}", typeStr);
            return nullptr;
        }

        std::shared_ptr<Asset> asset = nullptr;
        if (type == AssetType::Texture)
        {
            asset = std::make_shared<TextureAsset>();
        }
        else if (type == AssetType::Mesh)
        {
            asset = std::make_shared<StaticMeshAsset>();
        }
        else if (type == AssetType::Material)
        {
            asset = std::make_shared<MaterialAsset>();
        }

        if (!asset)
        {
            AP_ERROR("[AssetManager] Failed to construct asset type: {}", typeStr);
            return nullptr;
        }

        if (!asset->deserializeJson(json))
        {
            AP_ERROR("[AssetManager] Failed to deserialize asset: {}", assetPath.string());
            return nullptr;
        }

        asset->setAssetPath(assetPath.string());
        return asset;
    }

    auto AssetManager::sanitizeAssetName(std::string name) const -> std::string
    {
        if (name.empty())
        {
            return "material";
        }

        for (auto& ch : name)
        {
            if (ch == '\\' || ch == '/' || ch == ':' || ch == '*' || ch == '?' ||
                ch == '"' || ch == '<' || ch == '>' || ch == '|')
            {
                ch = '_';
            }
        }

        return name;
    }

    auto AssetManager::normalizePath(std::filesystem::path const& path) const -> std::string
    {
        if (path.empty())
        {
            return {};
        }

        auto ec = std::error_code{};
        auto normalized = std::filesystem::weakly_canonical(path, ec);
        if (ec)
        {
            return path.lexically_normal().string();
        }

        return normalized.string();
    }

    auto AssetManager::buildSourceKey(AssetType type, std::filesystem::path const& path) const -> std::string
    {
        return std::format("{}|{}", static_cast<int>(type), normalizePath(path));
    }

    auto AssetManager::ensureImported(Asset const& asset) -> std::optional<std::string>
    {
        auto importer = m_importers.findImporter(asset.getType());
        if (!importer)
        {
            AP_WARN("[AssetManager] No importer for asset type: {}", static_cast<int>(asset.getType()));
            return std::nullopt;
        }

        auto deps = DepRecorder{};
        auto forceReimport = false;
        {
            auto lock = std::scoped_lock{m_mutex};
            forceReimport = m_dirtyAssets.contains(asset.getHandle());
        }
        auto context = ImportContext{
            asset,
            asset.getAssetPath(),
            asset.getSourcePath(),
            m_targetProfile,
            m_ddc,
            deps,
            forceReimport
        };

        auto recordOpt = m_registry.findRecord(asset.getHandle());
        auto record = recordOpt.value_or(AssetRecord{});
        record.guid = asset.getHandle();
        record.assetPath = asset.getAssetPath();
        record.type = asset.getType();

        auto result = importer->import(context);
        auto targetId = m_targetProfile.toId();
        auto previousFingerprint = record.lastFingerprint.contains(targetId)
            ? record.lastFingerprint.at(targetId)
            : std::string{};

        if (!result.errors.empty())
        {
            AP_ERROR("[AssetManager] Import failed for asset: {}", asset.getAssetPath());
            record.lastImportFailed = true;
            record.lastErrorSummary = result.errors.front();
            m_registry.updateRecord(std::move(record));

            auto existing = record.ddcKeys.find(targetId);
            if (existing != record.ddcKeys.end() && !existing->second.empty())
            {
                return existing->second.front();
            }
            return std::nullopt;
        }

        record.deps = deps.deps;
        record.lastImportFailed = false;
        record.lastErrorSummary.clear();
        record.ddcKeys[targetId] = result.producedKeys;
        if (!result.producedKeys.empty())
        {
            record.lastFingerprint[targetId] = result.producedKeys.front();
        }

        record.lastSourceHash[targetId] = hashFileContents(asset.getSourcePath());

        m_registry.updateRecord(std::move(record));

        if (!result.producedKeys.empty() && record.lastFingerprint[targetId] != previousFingerprint)
        {
            markDependentsDirty(asset.getHandle());
        }

        if (forceReimport)
        {
            auto lock = std::scoped_lock{m_mutex};
            m_dirtyAssets.erase(asset.getHandle());
        }

        if (result.producedKeys.empty())
        {
            return std::nullopt;
        }

        return result.producedKeys.front();
    }
} // namespace april::asset
