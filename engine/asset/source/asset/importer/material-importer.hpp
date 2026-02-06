#pragma once

#include "importer.hpp"

namespace april::asset
{
    class MaterialImporter final : public IImporter
    {
    public:
        auto id() const -> std::string_view override { return "MaterialImporter"; }
        auto version() const -> int override { return 1; }
        auto supportsExtension(std::string_view /*extension*/) const -> bool override { return false; }
        auto primaryType() const -> AssetType override { return AssetType::Material; }

        auto import(ImportSourceContext const& context) -> ImportSourceResult override;
        auto cook(ImportCookContext const& context) -> ImportCookResult override;
    };
} // namespace april::asset
