#pragma once

#include "../blob-header.hpp"
#include "../material-asset.hpp"
#include "../static-mesh-asset.hpp"

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

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

    class GltfImporter
    {
    public:
        [[nodiscard]] auto importMesh(
            std::filesystem::path const& sourcePath,
            MeshImportSettings const& settings
        ) const -> std::optional<GltfMeshData>;

        [[nodiscard]] auto importMaterials(
            std::filesystem::path const& sourcePath
        ) const -> std::optional<std::vector<GltfMaterialData>>;
    };
} // namespace april::asset
