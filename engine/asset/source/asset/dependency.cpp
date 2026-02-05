#include "dependency.hpp"

namespace april::asset
{
    auto to_json(nlohmann::json& j, Dependency const& dep) -> void
    {
        j["kind"] = dep.kind == DepKind::Strong ? "Strong" : "Weak";
        j["asset"] = dep.asset;
    }

    auto from_json(nlohmann::json const& j, Dependency& dep) -> void
    {
        if (j.contains("kind"))
        {
            auto kind = j.at("kind").get<std::string>();
            dep.kind = kind == "Weak" ? DepKind::Weak : DepKind::Strong;
        }
        if (j.contains("asset"))
        {
            dep.asset = j.at("asset").get<AssetRef>();
        }
    }
} // namespace april::asset
