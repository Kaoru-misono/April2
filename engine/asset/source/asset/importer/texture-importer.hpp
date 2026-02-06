#pragma once

#include "importer.hpp"

namespace april::asset
{
    class TextureImporter final : public IImporter
    {
    public:
        auto id() const -> std::string_view override { return "TextureImporter"; }
        auto version() const -> int override { return 1; }
        auto supportsExtension(std::string_view extension) const -> bool override;
        auto primaryType() const -> AssetType override { return AssetType::Texture; }

        auto import(ImportSourceContext const& context) -> ImportSourceResult override;
        auto cook(ImportCookContext const& context) -> ImportCookResult override;
    };
} // namespace april::asset
