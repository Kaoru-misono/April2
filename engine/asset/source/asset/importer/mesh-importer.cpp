#include "mesh-importer.hpp"

#include <core/log/logger.hpp>

namespace april::asset
{
    auto MeshImporter::import(ImportSourceContext const& /*context*/) -> ImportSourceResult
    {
        auto result = ImportSourceResult{};
        result.errors.push_back("MeshImporter is deprecated. Use GltfImporter for GLTF/GLB files.");
        AP_WARN("[MeshImporter] This importer is deprecated. Use GltfImporter instead.");
        return result;
    }

    auto MeshImporter::cook(ImportCookContext const& /*context*/) -> ImportCookResult
    {
        auto result = ImportCookResult{};
        result.errors.push_back("MeshImporter is deprecated. Use GltfImporter for GLTF/GLB files.");
        AP_WARN("[MeshImporter] This importer is deprecated. Use GltfImporter instead.");
        return result;
    }
} // namespace april::asset
