#include "texture-asset.hpp"

#include <core/tools/hash.hpp>
#include <filesystem>

namespace april::asset
{
    static constexpr std::string_view TEXTURE_COMPILER_VERSION = "v1.0.0";

    auto TextureAsset::computeDDCKey() const -> std::string
    {
        std::string rawKeyData;

        rawKeyData += TEXTURE_COMPILER_VERSION;
        rawKeyData += "|";

        rawKeyData += m_settings.compression + "|";
        rawKeyData += (m_settings.sRGB ? "sRGB_ON" : "sRGB_OFF");
        rawKeyData += "|";
        rawKeyData += (m_settings.generateMips ? "Mips_ON" : "Mips_OFF");
        rawKeyData += "|";

        try
        {
            if (std::filesystem::exists(m_sourcePath))
            {
                auto ftime = std::filesystem::last_write_time(m_sourcePath);
                auto timestamp = ftime.time_since_epoch().count();
                rawKeyData += std::to_string(timestamp);
            }
            else
            {
                rawKeyData += "MISSING_FILE";
            }
        }
        catch (...)
        {
            rawKeyData += "ERROR_TIME";
        }

        return core::computeStringHash(rawKeyData);
    }
}
