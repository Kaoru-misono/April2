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

        auto appendTimestamp = [&](std::string_view label, std::string const& path) -> void
        {
            rawKeyData += std::string{label};
            rawKeyData += "=";

            try
            {
                if (!path.empty() && std::filesystem::exists(path))
                {
                    auto ftime = std::filesystem::last_write_time(path);
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

            rawKeyData += "|";
        };

        appendTimestamp("source", m_sourcePath);
        appendTimestamp("asset", m_assetPath);

        return core::computeStringHash(rawKeyData);
    }
}
