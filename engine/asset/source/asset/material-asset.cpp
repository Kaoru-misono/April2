#include "material-asset.hpp"

#include "material-asset.hpp"

#include <algorithm>

namespace april::asset
{
    auto to_json(nlohmann::json& j, MaterialParameters const& p) -> void
    {
        j["baseColorFactor"] = p.baseColorFactor;
        j["metallicFactor"] = p.metallicFactor;
        j["roughnessFactor"] = p.roughnessFactor;
        j["emissiveFactor"] = p.emissiveFactor;
        j["occlusionStrength"] = p.occlusionStrength;
        j["normalScale"] = p.normalScale;
        j["alphaCutoff"] = p.alphaCutoff;
        j["alphaMode"] = p.alphaMode;
        j["doubleSided"] = p.doubleSided;
    }

    auto from_json(nlohmann::json const& j, MaterialParameters& p) -> void
    {
        if (j.contains("baseColorFactor")) j.at("baseColorFactor").get_to(p.baseColorFactor);
        if (j.contains("metallicFactor")) p.metallicFactor = j.at("metallicFactor").get<float>();
        if (j.contains("roughnessFactor")) p.roughnessFactor = j.at("roughnessFactor").get<float>();
        if (j.contains("emissiveFactor")) j.at("emissiveFactor").get_to(p.emissiveFactor);
        if (j.contains("occlusionStrength")) p.occlusionStrength = j.at("occlusionStrength").get<float>();
        if (j.contains("normalScale")) p.normalScale = j.at("normalScale").get<float>();
        if (j.contains("alphaCutoff")) p.alphaCutoff = j.at("alphaCutoff").get<float>();
        if (j.contains("alphaMode")) p.alphaMode = j.at("alphaMode").get<std::string>();
        if (j.contains("doubleSided")) p.doubleSided = j.at("doubleSided").get<bool>();
    }

    auto to_json(nlohmann::json& j, TextureReference const& ref) -> void
    {
        j["asset"] = ref.asset;
        j["texCoord"] = ref.texCoord;
    }

    auto from_json(nlohmann::json const& j, TextureReference& ref) -> void
    {
        if (j.contains("asset")) ref.asset = j.at("asset").get<AssetRef>();
        if (j.contains("texCoord")) ref.texCoord = j.at("texCoord").get<int>();
    }

    auto to_json(nlohmann::json& j, MaterialTextures const& t) -> void
    {
        j = nlohmann::json::object();
        if (t.baseColorTexture) j["baseColorTexture"] = *t.baseColorTexture;
        if (t.metallicRoughnessTexture) j["metallicRoughnessTexture"] = *t.metallicRoughnessTexture;
        if (t.normalTexture) j["normalTexture"] = *t.normalTexture;
        if (t.occlusionTexture) j["occlusionTexture"] = *t.occlusionTexture;
        if (t.emissiveTexture) j["emissiveTexture"] = *t.emissiveTexture;
    }

    auto from_json(nlohmann::json const& j, MaterialTextures& t) -> void
    {
        if (j.contains("baseColorTexture"))
            t.baseColorTexture = j.at("baseColorTexture").get<TextureReference>();
        if (j.contains("metallicRoughnessTexture"))
            t.metallicRoughnessTexture = j.at("metallicRoughnessTexture").get<TextureReference>();
        if (j.contains("normalTexture"))
            t.normalTexture = j.at("normalTexture").get<TextureReference>();
        if (j.contains("occlusionTexture"))
            t.occlusionTexture = j.at("occlusionTexture").get<TextureReference>();
        if (j.contains("emissiveTexture"))
            t.emissiveTexture = j.at("emissiveTexture").get<TextureReference>();
    }

    auto MaterialAsset::serializeJson(nlohmann::json& outJson) -> void
    {
        rebuildReferences();
        Asset::serializeJson(outJson);

        outJson["parameters"] = parameters;
        outJson["textures"] = textures;
    }

    auto MaterialAsset::deserializeJson(nlohmann::json const& inJson) -> bool
    {
        if (!Asset::deserializeJson(inJson)) return false;

        if (inJson.contains("parameters"))
        {
            parameters = inJson["parameters"].get<MaterialParameters>();
        }

        if (inJson.contains("textures"))
        {
            textures = inJson["textures"].get<MaterialTextures>();
        }

        rebuildReferences();

        return true;
    }

    auto MaterialAsset::rebuildReferences() -> void
    {
        auto refs = std::vector<AssetRef>{};

        auto pushTexture = [&](std::optional<TextureReference> const& texture) -> void
        {
            if (texture.has_value())
            {
                refs.push_back(texture.value().asset);
            }
        };

        pushTexture(textures.baseColorTexture);
        pushTexture(textures.metallicRoughnessTexture);
        pushTexture(textures.normalTexture);
        pushTexture(textures.occlusionTexture);
        pushTexture(textures.emissiveTexture);

        std::sort(refs.begin(), refs.end(),
            [](AssetRef const& lhs, AssetRef const& rhs)
            {
                auto lhsGuid = lhs.guid.toString();
                auto rhsGuid = rhs.guid.toString();
                if (lhsGuid != rhsGuid)
                {
                    return lhsGuid < rhsGuid;
                }
                return lhs.subId < rhs.subId;
            });

        refs.erase(std::unique(refs.begin(), refs.end(),
            [](AssetRef const& lhs, AssetRef const& rhs)
            {
                return lhs.guid == rhs.guid && lhs.subId == rhs.subId;
            }),
            refs.end());

        setReferences(std::move(refs));
    }
}
