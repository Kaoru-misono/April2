#pragma once

#include "asset.hpp"
#include "texture-asset.hpp"
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

            // Cache the asset
            m_loadedAssets[asset->getHandle()] = asset;

            AP_INFO("[AssetManager] Loaded asset: {} ({})", assetPath.string(), asset->getHandle().toString());

            return asset;
        }
    };

} // namespace april::asset
