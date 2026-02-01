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

        void serializeJson(nlohmann::json& outJson) override
        {
            Asset::serializeJson(outJson);

            outJson["settings"] = m_settings;
        }

        bool deserializeJson(const nlohmann::json& inJson) override
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
