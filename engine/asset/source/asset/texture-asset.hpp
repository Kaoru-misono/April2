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

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(TextureImportSettings, sRGB, generateMips, compression, brightness)
    };

    class TextureAsset : public Asset
    {
    public:
        TextureAsset() : Asset(AssetType::Texture) {}

        TextureImportSettings m_settings;

        auto serializeJson(nlohmann::json& outJson) -> void override
        {
            Asset::serializeJson(outJson);

            outJson["settings"] = m_settings;
        }

        auto deserializeJson(nlohmann::json const& inJson) -> bool override
        {
            if (!Asset::deserializeJson(inJson)) return false;

            if (inJson.contains("settings"))
            {
                m_settings = inJson["settings"].get<TextureImportSettings>();
            }
            return true;
        }

        [[nodiscard]] auto computeDDCKey() const -> std::string;
    };
}
