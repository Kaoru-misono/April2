#include "static-mesh-asset.hpp"

#include <core/tools/hash.hpp>
#include <filesystem>
#include <format>

namespace april::asset
{
    static constexpr std::string_view MESH_COMPILER_VERSION = "v1.0.0";

    auto StaticMeshAsset::computeDDCKey() const -> std::string
    {
        auto rawKeyData = std::string{};

        rawKeyData += MESH_COMPILER_VERSION;
        rawKeyData += "|";

        rawKeyData += (m_settings.optimize ? "OPT_ON" : "OPT_OFF");
        rawKeyData += "|";
        rawKeyData += (m_settings.generateTangents ? "TANGENTS_ON" : "TANGENTS_OFF");
        rawKeyData += "|";
        rawKeyData += (m_settings.flipWindingOrder ? "FLIP_ON" : "FLIP_OFF");
        rawKeyData += "|";
        rawKeyData += std::format("{:.6f}", m_settings.scale);
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
