#pragma once

#include "asset.hpp"

namespace april::asset
{
    struct MeshImportSettings
    {
        bool optimize = true;           // Use meshoptimizer
        bool generateTangents = true;   // Compute tangent space
        bool flipWindingOrder = false;  // Flip triangle winding
        float scale = 1.0f;             // Uniform scale factor

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(MeshImportSettings, optimize, generateTangents, flipWindingOrder, scale)
    };

    class StaticMeshAsset : public Asset
    {
    public:
        StaticMeshAsset() : Asset(AssetType::Mesh) {}

        MeshImportSettings m_settings;

        auto serializeJson(nlohmann::json& outJson)  -> void override
        {
            Asset::serializeJson(outJson);

            outJson["settings"] = m_settings;
        }

        auto deserializeJson(nlohmann::json const& inJson) -> bool  override
        {
            if (!Asset::deserializeJson(inJson)) return false;

            if (inJson.contains("settings"))
            {
                m_settings = inJson["settings"].get<MeshImportSettings>();
            }
            return true;
        }

        [[nodiscard]] auto computeDDCKey() const -> std::string;
    };
}
