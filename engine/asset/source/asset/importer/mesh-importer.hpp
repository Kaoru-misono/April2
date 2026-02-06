#pragma once

#include "importer.hpp"

namespace april::asset
{
    // MeshImporter is deprecated in favor of GltfImporter.
    // Kept as placeholder for potential future OBJ/FBX support.
    class [[deprecated]] MeshImporter final : public IImporter
    {
    public:
        auto id() const -> std::string_view override { return "MeshImporter"; }
        auto version() const -> int override { return 1; }
        auto supportsExtension(std::string_view /*extension*/) const -> bool override { return false; }
        auto primaryType() const -> AssetType override { return AssetType::Mesh; }

        auto import(ImportSourceContext const& context) -> ImportSourceResult override;
        auto cook(ImportCookContext const& context) -> ImportCookResult override;
    };
} // namespace april::asset
