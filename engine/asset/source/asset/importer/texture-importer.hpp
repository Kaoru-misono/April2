#pragma once

#include "importer.hpp"

namespace april::asset
{
    class TextureImporter final : public IImporter
    {
    public:
        auto id() const -> std::string_view override { return "TextureImporter"; }
        auto version() const -> int override { return 1; }
        auto supports(AssetType type) const -> bool override { return type == AssetType::Texture; }

        auto import(ImportContext const& context) -> ImportResult override;
    };
} // namespace april::asset
