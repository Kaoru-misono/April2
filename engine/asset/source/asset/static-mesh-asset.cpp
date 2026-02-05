#include "static-mesh-asset.hpp"

#include <algorithm>

namespace april::asset
{
    auto to_json(nlohmann::json& j, MaterialSlot const& slot) -> void
    {
        j["name"] = slot.name;
        j["materialRef"] = slot.materialRef;
    }

    auto from_json(nlohmann::json const& j, MaterialSlot& slot) -> void
    {
        if (j.contains("name")) slot.name = j.at("name").get<std::string>();
        if (j.contains("materialRef")) slot.materialRef = j.at("materialRef").get<AssetRef>();
    }

    auto to_json(nlohmann::json& j, MeshImportSettings const& settings) -> void
    {
        j["optimize"] = settings.optimize;
        j["generateTangents"] = settings.generateTangents;
        j["flipWindingOrder"] = settings.flipWindingOrder;
        j["scale"] = settings.scale;
    }

    auto from_json(nlohmann::json const& j, MeshImportSettings& settings) -> void
    {
        if (j.contains("optimize")) settings.optimize = j.at("optimize").get<bool>();
        if (j.contains("generateTangents")) settings.generateTangents = j.at("generateTangents").get<bool>();
        if (j.contains("flipWindingOrder")) settings.flipWindingOrder = j.at("flipWindingOrder").get<bool>();
        if (j.contains("scale")) settings.scale = j.at("scale").get<float>();
    }

    auto StaticMeshAsset::serializeJson(nlohmann::json& outJson) -> void
    {
        rebuildReferences();
        Asset::serializeJson(outJson);

        outJson["settings"] = m_settings;
        outJson["materialSlots"] = m_materialSlots;
    }

    auto StaticMeshAsset::deserializeJson(nlohmann::json const& inJson) -> bool
    {
        if (!Asset::deserializeJson(inJson)) return false;

        if (inJson.contains("settings"))
        {
            m_settings = inJson["settings"].get<MeshImportSettings>();
        }

        if (inJson.contains("materialSlots"))
        {
            m_materialSlots = inJson["materialSlots"].get<std::vector<MaterialSlot>>();
        }

        rebuildReferences();
        return true;
    }

    auto StaticMeshAsset::rebuildReferences() -> void
    {
        auto refs = std::vector<AssetRef>{};
        refs.reserve(m_materialSlots.size());

        for (auto const& slot : m_materialSlots)
        {
            refs.push_back(slot.materialRef);
        }

        std::sort(refs.begin(), refs.end(),
            [](AssetRef const& lhs, AssetRef const& rhs) {
                auto lhsGuid = lhs.guid.toString();
                auto rhsGuid = rhs.guid.toString();
                if (lhsGuid != rhsGuid)
                {
                    return lhsGuid < rhsGuid;
                }
                return lhs.subId < rhs.subId;
            }
        );

        refs.erase(std::unique(refs.begin(), refs.end(),
            [](AssetRef const& lhs, AssetRef const& rhs) {
                return lhs.guid == rhs.guid && lhs.subId == rhs.subId;
            }),
            refs.end()
        );

        setReferences(std::move(refs));
    }
} // namespace april::asset
