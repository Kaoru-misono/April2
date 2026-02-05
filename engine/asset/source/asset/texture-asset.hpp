#pragma once

#include "asset.hpp"

namespace april::asset
{
    struct TextureImportSettings
    {
        bool sRGB = true;
        bool generateMips = true;
        std::string compression = "BC7";
        float brightness = 1.0f;
    };

    auto to_json(nlohmann::json& j, TextureImportSettings const& settings) -> void;
    auto from_json(nlohmann::json const& j, TextureImportSettings& settings) -> void;

    class TextureAsset : public Asset
    {
    public:
        TextureAsset() : Asset(AssetType::Texture) {}

        TextureImportSettings m_settings;

        auto serializeJson(nlohmann::json& outJson) -> void override;
        auto deserializeJson(nlohmann::json const& inJson) -> bool override;
    };
}
