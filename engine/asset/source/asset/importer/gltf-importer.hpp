#pragma once

#include "importer.hpp"
#include "../blob-header.hpp"
#include "../material-asset.hpp"
#include "../static-mesh-asset.hpp"

#include <array>
#include <filesystem>
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
        auto version() const -> int override { return 1; }
        auto supports(AssetType type) const -> bool override;
        auto import(ImportContext const& context) -> ImportResult override;

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
            ImportContext const& context
        ) const -> std::unordered_map<std::string, AssetRef>;

        // Import materials with texture references
        auto importMaterialAssets(
            std::vector<GltfMaterialData> const& materials,
            std::unordered_map<std::string, AssetRef> const& textureRefs,
            std::filesystem::path const& baseDir,
            ImportContext const& context
        ) const -> std::vector<MaterialSlot>;

        // Compile mesh data to DDC
        auto compileMesh(
            GltfMeshData const& meshData,
            ImportContext const& context
        ) const -> std::string;
    };
} // namespace april::asset
