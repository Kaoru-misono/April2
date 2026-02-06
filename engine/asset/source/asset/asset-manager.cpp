#include "asset-manager.hpp"

#include "ddc/ddc-utils.hpp"
#include "importer/texture-importer.hpp"
#include "importer/gltf-importer.hpp"
#include "importer/material-importer.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <system_error>
#include <string_view>
#include <utility>

namespace april::asset
{
    namespace
    {
        auto extractImporterId(std::string const& chain) -> std::string_view
        {
            if (chain.empty())
            {
                return {};
            }

            auto chainView = std::string_view{chain};
            auto lastSep = chainView.rfind('|');
            if (lastSep != std::string_view::npos)
            {
                chainView.remove_prefix(lastSep + 1);
            }

            auto atPos = chainView.rfind('@');
            if (atPos == std::string_view::npos)
            {
                return chainView;
            }

            return chainView.substr(0, atPos);
        }
    }

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
        return importAssetInternal(sourcePath, policy, "");
    }

    auto AssetManager::importAssetInternal(
        std::filesystem::path const& sourcePath,
        ImportPolicy policy,
        std::string const& parentImporterChain
    ) -> std::shared_ptr<Asset>
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

        auto* importer = m_importers.findImporterByExtension(extension);
        if (!importer)
        {
            AP_WARN("[AssetManager] Import skipped: Unsupported file extension '{}' for {}", extension, sourcePath.string());
            return nullptr;
        }

        auto assetFilePath = std::filesystem::path{sourcePath.string() + ".asset"};

        auto existingAsset = std::shared_ptr<Asset>{};
        auto const primaryType = importer->primaryType();

        if (std::filesystem::exists(assetFilePath))
        {
            if (primaryType == AssetType::Texture)
            {
                existingAsset = std::static_pointer_cast<Asset>(loadAssetFromFile<TextureAsset>(assetFilePath));
            }
            else if (primaryType == AssetType::Material)
            {
                existingAsset = std::static_pointer_cast<Asset>(loadAssetFromFile<MaterialAsset>(assetFilePath));
            }
            else if (primaryType == AssetType::Mesh)
            {
                existingAsset = std::static_pointer_cast<Asset>(loadAssetFromFile<StaticMeshAsset>(assetFilePath));
            }

            if (policy == ImportPolicy::ReuseIfExists && existingAsset)
            {
                return existingAsset;
            }

            if (policy == ImportPolicy::ReimportIfSourceChanged && existingAsset)
            {
                auto recordOpt = m_registry.findRecord(existingAsset->getHandle());
                if (recordOpt.has_value())
                {
                    auto currentHash = hashFileContents(sourcePath.string());
                    if (!recordOpt->lastSourceHash.empty() && recordOpt->lastSourceHash == currentHash)
                    {
                        return existingAsset;
                    }
                }
            }
        }

        auto importerChain = appendImporterChain(parentImporterChain, importer->id(), importer->version());

        auto context = ImportSourceContext{};
        context.sourcePath = sourcePath;
        context.importerChain = importerChain;
        context.findAssetBySource = [this](std::filesystem::path const& path, AssetType type) -> std::shared_ptr<Asset>
        {
            return findAssetBySourcePath(path, type);
        };
        context.importSubAsset = [this, importerChain](std::filesystem::path const& path) -> std::shared_ptr<Asset>
        {
            return importAssetInternal(path, ImportPolicy::ReuseIfExists, importerChain);
        };

        auto result = importer->import(context);
        if (!result.errors.empty())
        {
            for (auto const& error : result.errors)
            {
                AP_ERROR("[AssetManager] Import error ({}): {}", importer->id(), error);
            }
            return nullptr;
        }

        for (auto const& warning : result.warnings)
        {
            AP_WARN("[AssetManager] Import warning ({}): {}", importer->id(), warning);
        }

        auto primaryAsset = result.primaryAsset;
        if (!primaryAsset && !result.assets.empty())
        {
            primaryAsset = result.assets.front();
        }

        if (!primaryAsset)
        {
            AP_WARN("[AssetManager] Import produced no assets for {}", sourcePath.string());
            return nullptr;
        }

        auto assetsToSave = result.assets;
        if (std::find(assetsToSave.begin(), assetsToSave.end(), primaryAsset) == assetsToSave.end())
        {
            assetsToSave.push_back(primaryAsset);
        }

        for (auto const& asset : assetsToSave)
        {
            if (!asset)
            {
                continue;
            }

            if (asset->getSourcePath().empty())
            {
                asset->setSourcePath(sourcePath.string());
            }

            asset->setImporterChain(importerChain);

            if (asset->getAssetPath().empty())
            {
                if (asset == primaryAsset)
                {
                    asset->setAssetPath(assetFilePath.string());
                }
                else
                {
                    AP_WARN("[AssetManager] Skipping asset with empty path for {}", sourcePath.string());
                    continue;
                }
            }

            if (!saveAssetFile(asset, asset->getAssetPath()))
            {
                return nullptr;
            }

            if (!asset->getSourcePath().empty())
            {
                auto recordOpt = m_registry.findRecord(asset->getHandle());
                auto record = recordOpt.value_or(AssetRecord{});
                record.guid = asset->getHandle();
                record.assetPath = asset->getAssetPath();
                record.type = asset->getType();

                record.lastSourceHash = hashFileContents(asset->getSourcePath());

                m_registry.updateRecord(std::move(record));
            }
        }

        AP_INFO("[AssetManager] Imported asset: {} -> {} (UUID: {})",
            sourcePath.string(), primaryAsset->getAssetPath(), primaryAsset->getHandle().toString()
        );

        return primaryAsset;
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
                return std::static_pointer_cast<Asset>(loadAssetFromFile<TextureAsset>(*assetPathFromIndex));
            }
            if (type == AssetType::Material)
            {
                return std::static_pointer_cast<Asset>(loadAssetFromFile<MaterialAsset>(*assetPathFromIndex));
            }
            if (type == AssetType::Mesh)
            {
                return std::static_pointer_cast<Asset>(loadAssetFromFile<StaticMeshAsset>(*assetPathFromIndex));
            }
        }

        // Check if asset file exists and load it
        auto assetFilePath = sourcePath;
        if (type == AssetType::Texture)
        {
            assetFilePath = std::filesystem::path{assetFilePath.string() + ".asset"};
        }

        if (std::filesystem::exists(assetFilePath))
        {
            if (type == AssetType::Texture)
            {
                return std::static_pointer_cast<Asset>(loadAssetFromFile<TextureAsset>(assetFilePath));
            }
            else if (type == AssetType::Material)
            {
                return std::static_pointer_cast<Asset>(loadAssetFromFile<MaterialAsset>(assetFilePath));
            }
            else if (type == AssetType::Mesh)
            {
                return std::static_pointer_cast<Asset>(loadAssetFromFile<StaticMeshAsset>(assetFilePath));
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

        return saveAssetFile(material, outputPath);
    }

    auto AssetManager::saveAssetFile(
        std::shared_ptr<Asset> const& asset,
        std::filesystem::path const& assetPath
    ) -> bool
    {
        if (!asset)
        {
            AP_ERROR("[AssetManager] Cannot save null asset");
            return false;
        }

        if (assetPath.empty())
        {
            AP_ERROR("[AssetManager] Cannot save asset with empty path");
            return false;
        }

        asset->setAssetPath(assetPath.string());

        auto json = nlohmann::json{};
        asset->serializeJson(json);

        auto file = std::ofstream{assetPath};
        if (!file.is_open())
        {
            AP_ERROR("[AssetManager] Failed to write asset file: {}", assetPath.string());
            return false;
        }

        file << json.dump(4);
        file.close();

        registerAssetInternal(asset, assetPath, true);

        AP_INFO("[AssetManager] Saved asset: {} (UUID: {})",
            assetPath.string(), asset->getHandle().toString());
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
        auto importer = (IImporter*) nullptr;
        if (!asset.getImporterChain().empty())
        {
            auto importerId = extractImporterId(asset.getImporterChain());
            if (!importerId.empty())
            {
                importer = m_importers.findImporterById(importerId);
            }
        }
        if (!importer)
        {
            importer = m_importers.findImporter(asset.getType());
        }
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
        auto context = ImportCookContext{
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

        auto result = importer->cook(context);
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

        if (!asset.getSourcePath().empty())
        {
            record.lastSourceHash = hashFileContents(asset.getSourcePath());
        }

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
