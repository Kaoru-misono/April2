#include "mesh-importer.hpp"

#include <core/log/logger.hpp>

namespace april::asset
{
    auto MeshImporter::import(ImportContext const& /*context*/) -> ImportResult
    {
        // MeshImporter is deprecated in favor of GltfImporter.
        // This implementation returns an error for any import attempt.
        auto result = ImportResult{};
        result.errors.push_back("MeshImporter is deprecated. Use GltfImporter for GLTF/GLB files.");
        AP_WARN("[MeshImporter] This importer is deprecated. Use GltfImporter instead.");
        return result;
    }
} // namespace april::asset
