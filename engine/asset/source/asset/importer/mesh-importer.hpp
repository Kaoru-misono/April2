#pragma once

#include "importer.hpp"

namespace april::asset
{
    class MeshImporter final : public IImporter
    {
    public:
        auto id() const -> std::string_view override { return "MeshImporter"; }
        auto version() const -> int override { return 1; }
        auto supports(AssetType type) const -> bool override { return type == AssetType::Mesh; }

        auto import(ImportContext const& context) -> ImportResult override;
    };
} // namespace april::asset
