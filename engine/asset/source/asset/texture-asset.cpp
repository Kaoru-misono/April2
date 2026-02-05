#include "texture-asset.hpp"

namespace april::asset
{
    auto to_json(nlohmann::json& j, TextureImportSettings const& settings) -> void
    {
        j["sRGB"] = settings.sRGB;
        j["generateMips"] = settings.generateMips;
        j["compression"] = settings.compression;
        j["brightness"] = settings.brightness;
    }

    auto from_json(nlohmann::json const& j, TextureImportSettings& settings) -> void
    {
        if (j.contains("sRGB")) settings.sRGB = j.at("sRGB").get<bool>();
        if (j.contains("generateMips")) settings.generateMips = j.at("generateMips").get<bool>();
        if (j.contains("compression")) settings.compression = j.at("compression").get<std::string>();
        if (j.contains("brightness")) settings.brightness = j.at("brightness").get<float>();
    }

    auto TextureAsset::serializeJson(nlohmann::json& outJson) -> void
    {
        Asset::serializeJson(outJson);

        outJson["settings"] = m_settings;
    }

    auto TextureAsset::deserializeJson(nlohmann::json const& inJson) -> bool
    {
        if (!Asset::deserializeJson(inJson)) return false;

        if (inJson.contains("settings"))
        {
            m_settings = inJson["settings"].get<TextureImportSettings>();
        }
        return true;
    }
}
