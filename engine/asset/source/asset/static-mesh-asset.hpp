#pragma once

#include "asset.hpp"

#include <string>
#include <vector>

namespace april::asset
{
    struct MaterialSlot
    {
        std::string name{};
        AssetRef materialRef{};
    };

    auto to_json(nlohmann::json& j, MaterialSlot const& slot) -> void;
    auto from_json(nlohmann::json const& j, MaterialSlot& slot) -> void;

    struct MeshImportSettings
    {
        bool optimize = true;           // Use meshoptimizer
        bool generateTangents = true;   // Compute tangent space
        bool flipWindingOrder = false;  // Flip triangle winding
        float scale = 1.0f;             // Uniform scale factor
    };

    auto to_json(nlohmann::json& j, MeshImportSettings const& settings) -> void;
    auto from_json(nlohmann::json const& j, MeshImportSettings& settings) -> void;

    class StaticMeshAsset : public Asset
    {
    public:
        StaticMeshAsset() : Asset(AssetType::Mesh) {}

        MeshImportSettings m_settings;
        std::vector<MaterialSlot> m_materialSlots{};

        auto serializeJson(nlohmann::json& outJson)  -> void override;
        auto deserializeJson(nlohmann::json const& inJson) -> bool  override;

    private:
        auto rebuildReferences() -> void;
    };
}
