#include "asset.hpp"

#include <core/error/assert.hpp>

namespace april::asset
{
    auto Asset::serializeJson(nlohmann::json& outJson) -> void
    {
        outJson["guid"] = m_handle.toString();

        outJson["type"] = m_type;

        outJson["source_path"] = m_sourcePath;
    }

    auto Asset::deserializeJson(const nlohmann::json& inJson) -> bool
    {
        if (inJson.contains("guid"))
        {
            m_handle = UUID(inJson["guid"].get<std::string>());
        }

        if (inJson.contains("type"))
        {
            AssetType jsonType = inJson["type"].get<AssetType>();
            AP_ASSERT(jsonType == m_type, "Asset type mismatch in JSON!");
        }

        if (inJson.contains("source_path"))
        {
            m_sourcePath = inJson["source_path"].get<std::string>();
        }

        return true;
    }
}
