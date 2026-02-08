#pragma once

#include "importer.hpp"
#include "../blob-header.hpp"
#include "../material-asset.hpp"
#include "../static-mesh-asset.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace tinygltf
{
    class Model;
}

namespace april::asset
{
    struct GltfTextureSource
    {
        std::filesystem::path path{};
        int texCoord{0};
    };

    struct GltfMaterialData
    {
        std::string name{};
        MaterialParameters parameters{};

        std::optional<GltfTextureSource> baseColorTexture{};
        std::optional<GltfTextureSource> metallicRoughnessTexture{};
        std::optional<GltfTextureSource> normalTexture{};
        std::optional<GltfTextureSource> occlusionTexture{};
        std::optional<GltfTextureSource> emissiveTexture{};
    };

    struct GltfMeshData
    {
        std::vector<float> vertices{};
        std::vector<uint32_t> indices{};
        std::vector<Submesh> submeshes{};
        std::array<float, 3> boundsMin{};
        std::array<float, 3> boundsMax{};
    };

    class GltfImporter final : public IImporter
    {
    public:
        auto id() const -> std::string_view override { return "GltfImporter"; }
        auto version() const -> int override { return 2; }
        auto supportsExtension(std::string_view extension) const -> bool override;
        auto primaryType() const -> AssetType override { return AssetType::Mesh; }
        auto import(ImportSourceContext const& context) -> ImportSourceResult override;
        auto cook(ImportCookContext const& context) -> ImportCookResult override;

        // Legacy API for direct mesh/material extraction (used internally)
        [[nodiscard]] auto importMesh(
            std::filesystem::path const& sourcePath,
            MeshImportSettings const& settings
        ) const -> std::optional<GltfMeshData>;

        [[nodiscard]] auto importMaterials(
            std::filesystem::path const& sourcePath
        ) const -> std::optional<std::vector<GltfMaterialData>>;

    private:
        // Import textures from material data, with deduplication
        auto importTextures(
            std::vector<GltfMaterialData> const& materials,
            ImportSourceContext const& context
        ) const -> std::unordered_map<std::string, AssetRef>;

        // Import materials with texture references
        auto importMaterialAssets(
            std::vector<GltfMaterialData> const& materials,
            std::unordered_map<std::string, AssetRef> const& textureRefs,
            std::filesystem::path const& baseDir,
            ImportSourceContext const& context,
            std::vector<std::shared_ptr<MaterialAsset>>& outAssets
        ) const -> std::vector<MaterialSlot>;

        // Compile mesh data to DDC
        auto compileMesh(
            GltfMeshData const& meshData,
            ImportCookContext const& context
        ) const -> std::string;
    };
} // namespace april::asset
