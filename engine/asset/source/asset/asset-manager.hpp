#pragma once

#include "asset.hpp"
#include "texture-asset.hpp"
#include "static-mesh-asset.hpp"
#include "blob-header.hpp"
#include "ddc/ddc-manager.hpp"

#include <core/log/logger.hpp>
#include <core/tools/uuid.hpp>

#include <unordered_map>
#include <filesystem>
#include <memory>
#include <fstream>

namespace april::asset
{
    /**
     * AssetManager handles loading and caching of asset metadata and compiled data.
     * It is graphics-independent and works purely with CPU memory.
     */
    class AssetManager
    {
    public:
        explicit AssetManager(std::filesystem::path const& assetRoot = "Assets",
                              std::filesystem::path const& cacheRoot = "Cache/DDC")
            : m_assetRoot{assetRoot}
            , m_ddcManager{cacheRoot}
        {
            AP_INFO("[AssetManager] Initialized. Asset root: {}, Cache root: {}",
                    assetRoot.string(), cacheRoot.string());
        }

        ~AssetManager()
        {
            AP_INFO("[AssetManager] Shutdown.");
        }

        /**
         * Import a raw file (e.g., .png, .gltf) into the asset system.
         * Creates a corresponding .asset file alongside the source file.
         */
        [[nodiscard]] auto importAsset(std::filesystem::path const& sourcePath) -> std::shared_ptr<Asset>
        {
            if (!std::filesystem::exists(sourcePath))
            {
                AP_ERROR("[AssetManager] Import failed: Source file not found: {}", sourcePath.string());
                return nullptr;
            }

            auto extension = sourcePath.extension().string();
            // Convert extension to lowercase for robust check (optional but recommended)
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            std::shared_ptr<Asset> newAsset = nullptr;

            // 1. Identify Asset Type based on extension
            if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".tga")
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

            // 2. Determine output path for the .asset file (e.g., "Texture.png" -> "Texture.png.asset")
            auto assetFilePath = sourcePath;
            assetFilePath += ".asset"; // Append .asset to the existing extension

            if (std::filesystem::exists(assetFilePath))
            {
                if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".tga")
                {
                    return loadAssetFromFile<TextureAsset>(assetFilePath);
                }
                if (extension == ".gltf" || extension == ".glb")
                {
                    return loadAssetFromFile<StaticMeshAsset>(assetFilePath);
                }
            }

            // 3. Configure the new asset
            // Note: We store the relative path or absolute path depending on your project policy.
            // Here we store the path exactly as passed, but usually you want relative to ProjectRoot.
            newAsset->setSourcePath(sourcePath.string());
            newAsset->setAssetPath(assetFilePath.string());

            // 4. Serialize to JSON
            nlohmann::json json;
            newAsset->serializeJson(json);

            // 5. Write to disk
            auto file = std::ofstream{assetFilePath};
            if (!file.is_open())
            {
                AP_ERROR("[AssetManager] Failed to write .asset file: {}", assetFilePath.string());
                return nullptr;
            }
            file << json.dump(4); // Pretty print with 4 spaces indent
            file.close();

            // 6. Register and Cache
            auto handle = newAsset->getHandle();
            m_assetRegistry[handle] = assetFilePath;
            m_loadedAssets[handle] = newAsset;

            AP_INFO("[AssetManager] Imported asset: {} -> {} (UUID: {})",
                sourcePath.string(), assetFilePath.string(), handle.toString());

            return newAsset;
        }

        /**
         * Get an asset by UUID from cache or load from registry.
         */
        template <typename T>
        [[nodiscard]] auto getAsset(core::UUID handle) -> std::shared_ptr<T>
        {
            // Check memory cache
            if (m_loadedAssets.contains(handle))
            {
                return std::static_pointer_cast<T>(m_loadedAssets.at(handle));
            }

            // Not in memory? Check registry for file path
            if (m_assetRegistry.contains(handle))
            {
                auto const& path = m_assetRegistry.at(handle);
                return loadAssetFromFile<T>(path);
            }

            AP_ERROR("[AssetManager] Asset UUID not found in registry: {}", handle.toString());
            return nullptr;
        }

        /**
         * Load and cache an asset from a .asset file path.
         */
        template <typename T>
        [[nodiscard]] auto loadAsset(std::filesystem::path const& assetPath) -> std::shared_ptr<T>
        {
            return loadAssetFromFile<T>(assetPath);
        }

        /**
         * Get compiled texture data for a TextureAsset.
         * Returns a TexturePayload with header and pixel data span.
         * The blob is stored in the provided output vector for lifetime management.
         */
        [[nodiscard]] auto getTextureData(TextureAsset const& asset, std::vector<std::byte>& outBlob) -> TexturePayload
        {
            outBlob = m_ddcManager.getOrCompileTexture(asset);

            if (outBlob.size() < sizeof(TextureHeader))
            {
                AP_ERROR("[AssetManager] Invalid texture blob size for: {}", asset.getSourcePath());
                return {};
            }

            auto payload = TexturePayload{};

            // Parse header
            std::memcpy(&payload.header, outBlob.data(), sizeof(TextureHeader));

            if (!payload.header.isValid())
            {
                AP_ERROR("[AssetManager] Invalid texture header for: {}", asset.getSourcePath());
                return {};
            }

            // Create span for pixel data
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

        /**
         * Get compiled mesh data for a StaticMeshAsset.
         * Returns a MeshPayload with header, submeshes, vertex data, and index data spans.
         * The blob is stored in the provided output vector for lifetime management.
         */
        [[nodiscard]] auto getMeshData(StaticMeshAsset const& asset, std::vector<std::byte>& outBlob) -> MeshPayload
        {
            outBlob = m_ddcManager.getOrCompileMesh(asset);

            if (outBlob.size() < sizeof(MeshHeader))
            {
                AP_ERROR("[AssetManager] Invalid mesh blob size for: {}", asset.getSourcePath());
                return {};
            }

            auto payload = MeshPayload{};

            // Parse header
            std::memcpy(&payload.header, outBlob.data(), sizeof(MeshHeader));

            if (!payload.header.isValid())
            {
                AP_ERROR("[AssetManager] Invalid mesh header for: {}", asset.getSourcePath());
                return {};
            }

            auto offset = sizeof(MeshHeader);

            // Create span for submeshes
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

            // Create span for vertex data
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

            // Create span for index data
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

        /**
         * Register an asset path in the registry for UUID-based lookup.
         */
        auto registerAssetPath(core::UUID handle, std::filesystem::path const& path) -> void
        {
            m_assetRegistry[handle] = path;
            AP_INFO("[AssetManager] Registered asset: {} -> {}", handle.toString(), path.string());
        }

        /**
         * Scan a directory for .asset files and register them.
         */
        auto scanDirectory(std::filesystem::path const& directory) -> size_t
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
                    // Try to load the asset to get its UUID
                    auto asset = loadAssetMetadata(entry.path());
                    if (asset)
                    {
                        m_assetRegistry[asset->getHandle()] = entry.path();
                        ++count;
                    }
                }
            }

            AP_INFO("[AssetManager] Scanned directory '{}': found {} assets", directory.string(), count);
            return count;
        }

        [[nodiscard]] auto getDDCManager() -> DDCManager&
        {
            return m_ddcManager;
        }

    private:
        std::filesystem::path m_assetRoot;
        DDCManager m_ddcManager;

        // Cache: Assets currently in memory
        std::unordered_map<core::UUID, std::shared_ptr<Asset>> m_loadedAssets{};

        // Registry: Maps UUID -> Path to .asset file on disk
        std::unordered_map<core::UUID, std::filesystem::path> m_assetRegistry{};

        /**
         * Load asset metadata from a .asset JSON file.
         */
        [[nodiscard]] auto loadAssetMetadata(std::filesystem::path const& assetPath) -> std::shared_ptr<Asset>
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

            // Determine asset type
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
            // Add more types as needed

            if (type == AssetType::None)
            {
                AP_ERROR("[AssetManager] Unknown asset type: {}", typeStr);
                return nullptr;
            }

            return nullptr; // Base implementation - specialized by template
        }

        /**
         * Load a typed asset from file.
         */
        template <typename T>
        [[nodiscard]] auto loadAssetFromFile(std::filesystem::path const& assetPath) -> std::shared_ptr<T>
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

            auto asset = std::make_shared<T>();
            if (!asset->deserializeJson(json))
            {
                AP_ERROR("[AssetManager] Failed to deserialize asset: {}", assetPath.string());
                return nullptr;
            }

            asset->setAssetPath(assetPath.string());

            // Cache the asset
            m_loadedAssets[asset->getHandle()] = asset;

            AP_INFO("[AssetManager] Loaded asset: {} ({})", assetPath.string(), asset->getHandle().toString());

            return asset;
        }
    };

} // namespace april::asset
