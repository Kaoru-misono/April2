#include "asset-manager.hpp"
#include "asset-manager.hpp"

#include "ddc/ddc-utils.hpp"
#include "importer/texture-importer.hpp"
#include "importer/mesh-importer.hpp"
#include "importer/material-importer.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
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
        m_importers.registerImporter(std::make_unique<MeshImporter>());
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
            else if (extension == ".gltf" || extension == ".glb")
            {
                existingAsset = loadAssetFromFile<StaticMeshAsset>(assetFilePath);
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
        else if (extension == ".gltf" || extension == ".glb")
        {
            newAsset = std::make_shared<StaticMeshAsset>();
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

        if (extension == ".gltf" || extension == ".glb")
        {
            auto meshAsset = std::static_pointer_cast<StaticMeshAsset>(newAsset);
            importGltfMaterials(*meshAsset, sourcePath);
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
        m_assetRegistry[handle] = path;
        auto asset = loadAssetMetadata(path);
        if (asset)
        {
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

        m_assetRegistry[asset->getHandle()] = assetPath;
        if (cacheAsset)
        {
            m_loadedAssets[asset->getHandle()] = asset;
        }
        m_registry.registerAsset(*asset, assetPath.string());
    }

    auto AssetManager::markDependentsDirty(core::UUID const& guid) -> void
    {
        auto dependents = m_registry.getDependents(guid);
        for (auto const& dependent : dependents)
        {
            m_dirtyAssets.insert(dependent);
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

    auto AssetManager::importGltfMaterials(StaticMeshAsset& meshAsset, std::filesystem::path const& sourcePath) -> void
    {
        auto importer = GltfImporter{};
        auto materials = importer.importMaterials(sourcePath);

        meshAsset.m_materialSlots.clear();

        if (!materials)
        {
            return;
        }

        meshAsset.m_materialSlots.reserve(materials->size());
        auto baseDir = sourcePath.parent_path();

        for (auto const& materialData : *materials)
        {
            auto materialName = materialData.name;
            auto sanitizedName = sanitizeAssetName(materialName);
            auto materialPath = baseDir / (sanitizedName + ".material.asset");

            auto materialAsset = std::shared_ptr<MaterialAsset>{};
            if (std::filesystem::exists(materialPath))
            {
                materialAsset = loadAssetFromFile<MaterialAsset>(materialPath);
            }
            if (!materialAsset)
            {
                materialAsset = std::make_shared<MaterialAsset>();
            }

            materialAsset->setSourcePath(sourcePath.string());
            if (auto importer = m_importers.findImporter(AssetType::Material))
            {
                materialAsset->setImporter(std::string{importer->id()}, importer->version());
            }
            materialAsset->parameters = materialData.parameters;

            auto textures = MaterialTextures{};
            textures.baseColorTexture = importMaterialTexture(materialData.baseColorTexture);
            textures.metallicRoughnessTexture = importMaterialTexture(materialData.metallicRoughnessTexture);
            textures.normalTexture = importMaterialTexture(materialData.normalTexture);
            textures.occlusionTexture = importMaterialTexture(materialData.occlusionTexture);
            textures.emissiveTexture = importMaterialTexture(materialData.emissiveTexture);
            materialAsset->textures = textures;

            auto references = std::vector<AssetRef>{};
            if (textures.baseColorTexture) references.push_back(textures.baseColorTexture->asset);
            if (textures.metallicRoughnessTexture) references.push_back(textures.metallicRoughnessTexture->asset);
            if (textures.normalTexture) references.push_back(textures.normalTexture->asset);
            if (textures.occlusionTexture) references.push_back(textures.occlusionTexture->asset);
            if (textures.emissiveTexture) references.push_back(textures.emissiveTexture->asset);
            materialAsset->setReferences(std::move(references));

            if (!saveMaterialAsset(materialAsset, materialPath))
            {
                AP_WARN("[AssetManager] Failed to save material asset: {}", materialPath.string());
            }

            meshAsset.m_materialSlots.push_back(MaterialSlot{
                materialName,
                AssetRef{materialAsset->getHandle(), 0}
            });
        }

        auto meshRefs = std::vector<AssetRef>{};
        meshRefs.reserve(meshAsset.m_materialSlots.size());
        for (auto const& slot : meshAsset.m_materialSlots)
        {
            meshRefs.push_back(slot.materialRef);
        }
        meshAsset.setReferences(std::move(meshRefs));
    }

    auto AssetManager::importMaterialTexture(
        std::optional<GltfTextureSource> const& source
    ) -> std::optional<TextureReference>
    {
        if (!source)
        {
            return std::nullopt;
        }

        auto const& texturePath = source->path;
        if (!std::filesystem::exists(texturePath))
        {
            AP_WARN("[AssetManager] Texture file not found: {}", texturePath.string());
            return std::nullopt;
        }

        auto textureAsset = importAsset(texturePath);
        if (!textureAsset || textureAsset->getType() != AssetType::Texture)
        {
            AP_WARN("[AssetManager] Failed to import texture: {}", texturePath.string());
            return std::nullopt;
        }

        return TextureReference{AssetRef{textureAsset->getHandle(), 0}, source->texCoord};
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

    auto AssetManager::ensureImported(Asset const& asset) -> std::optional<std::string>
    {
        auto importer = m_importers.findImporter(asset.getType());
        if (!importer)
        {
            AP_WARN("[AssetManager] No importer for asset type: {}", static_cast<int>(asset.getType()));
            return std::nullopt;
        }

        auto deps = DepRecorder{};
        auto forceReimport = m_dirtyAssets.contains(asset.getHandle());
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
            m_dirtyAssets.erase(asset.getHandle());
        }

        if (result.producedKeys.empty())
        {
            return std::nullopt;
        }

        return result.producedKeys.front();
    }
} // namespace april::asset
