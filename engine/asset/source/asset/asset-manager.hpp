#pragma once
#pragma once

#include "asset.hpp"
#include "texture-asset.hpp"
#include "static-mesh-asset.hpp"
#include "material-asset.hpp"
#include "blob-header.hpp"
#include "ddc/local-ddc.hpp"
#include "asset-registry.hpp"
#include "importer/importer-registry.hpp"
#include "importer/gltf-importer.hpp"

#include <core/log/logger.hpp>
#include <core/tools/uuid.hpp>

#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <memory>
#include <fstream>
#include <optional>
#include <format>
#include <cstdint>

namespace april::asset
{
    /**
     * AssetManager handles loading and caching of asset metadata and compiled data.
     * It is graphics-independent and works purely with CPU memory.
     */
    class AssetManager
    {
    public:
        enum class ImportPolicy : uint8_t
        {
            ReuseIfExists,
            Reimport,
            ReimportIfSourceChanged
        };

        explicit AssetManager(
            std::filesystem::path const& assetRoot = "content",
            std::filesystem::path const& cacheRoot = "build/cache/DDC"
        );

        ~AssetManager();

        /**
         * Import a raw file (e.g., .png, .gltf) into the asset system.
         * Creates a corresponding .asset file alongside the source file.
         */
        [[nodiscard]] auto importAsset(
            std::filesystem::path const& sourcePath,
            ImportPolicy policy = ImportPolicy::ReuseIfExists
        ) -> std::shared_ptr<Asset>;

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
        [[nodiscard]] auto getTextureData(TextureAsset const& asset, std::vector<std::byte>& outBlob) -> TexturePayload;

        /**
         * Get compiled mesh data for a StaticMeshAsset.
         * Returns a MeshPayload with header, submeshes, vertex data, and index data spans.
         * The blob is stored in the provided output vector for lifetime management.
         */
        [[nodiscard]] auto getMeshData(StaticMeshAsset const& asset, std::vector<std::byte>& outBlob) -> MeshPayload;

        /**
         * Save a MaterialAsset to a .material.asset file.
         */
        auto saveMaterialAsset(std::shared_ptr<MaterialAsset> const& material, std::filesystem::path const& outputPath) -> bool;

        /**
         * Register an asset path in the registry for UUID-based lookup.
         */
        auto registerAssetPath(core::UUID handle, std::filesystem::path const& path) -> void;

        /**
         * Scan a directory for .asset files and register them.
         */
        auto scanDirectory(std::filesystem::path const& directory) -> size_t;

        [[nodiscard]] auto getDdc() -> LocalDdc&;

    private:
        std::filesystem::path m_assetRoot;
        LocalDdc m_ddc;
        AssetRegistry m_registry{};
        ImporterRegistry m_importers{};
        TargetProfile m_targetProfile{};

        // Cache: Assets currently in memory
        std::unordered_map<core::UUID, std::shared_ptr<Asset>> m_loadedAssets{};

        // Registry: Maps UUID -> Path to .asset file on disk
        std::unordered_map<core::UUID, std::filesystem::path> m_assetRegistry{};
        std::unordered_set<core::UUID> m_dirtyAssets{};

        /**
         * Load asset metadata from a .asset JSON file.
         */
        [[nodiscard]] auto loadAssetMetadata(std::filesystem::path const& assetPath) -> std::shared_ptr<Asset>;

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

            registerAssetInternal(asset, assetPath, true);

            AP_INFO("[AssetManager] Loaded asset: {} ({})", assetPath.string(), asset->getHandle().toString());

            return asset;
        }

        auto importGltfMaterials(StaticMeshAsset& meshAsset, std::filesystem::path const& sourcePath) -> void;

        auto importMaterialTexture(
            std::optional<GltfTextureSource> const& source
        ) -> std::optional<TextureReference>;

        auto registerAssetInternal(
            std::shared_ptr<Asset> const& asset,
            std::filesystem::path const& assetPath,
            bool cacheAsset
        ) -> void;

        auto markDependentsDirty(core::UUID const& guid) -> void;

        auto sanitizeAssetName(std::string name) const -> std::string;

        auto ensureImported(Asset const& asset) -> std::optional<std::string>;
    };

} // namespace april::asset
