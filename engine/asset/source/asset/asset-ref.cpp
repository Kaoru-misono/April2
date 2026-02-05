#include "asset-ref.hpp"

namespace april::asset
{
    auto to_json(nlohmann::json& j, AssetRef const& ref) -> void
    {
        j["guid"] = ref.guid.toString();
        j["subId"] = ref.subId;
    }

    auto from_json(nlohmann::json const& j, AssetRef& ref) -> void
    {
        if (j.contains("guid"))
        {
            ref.guid = core::UUID{j.at("guid").get<std::string>()};
        }
        if (j.contains("subId"))
        {
            ref.subId = j.at("subId").get<uint32_t>();
        }
    }
} // namespace april::asset
