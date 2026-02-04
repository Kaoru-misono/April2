#include "material-asset.hpp"

#include <core/tools/hash.hpp>
#include <format>

namespace april::asset
{
    static constexpr std::string_view MATERIAL_COMPILER_VERSION = "v1.0.0";

    auto MaterialAsset::serializeJson(nlohmann::json& outJson) -> void
    {
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

        return true;
    }

    auto MaterialAsset::computeDDCKey() const -> std::string
    {
        std::string rawKeyData;

        rawKeyData += MATERIAL_COMPILER_VERSION;
        rawKeyData += "|";

        // Include material parameters in key
        rawKeyData += std::format("baseColor={},{},{},{}|",
            parameters.baseColorFactor.x,
            parameters.baseColorFactor.y,
            parameters.baseColorFactor.z,
            parameters.baseColorFactor.w);

        rawKeyData += std::format("metallic={}|", parameters.metallicFactor);
        rawKeyData += std::format("roughness={}|", parameters.roughnessFactor);
        rawKeyData += std::format("alphaMode={}|", parameters.alphaMode);
        rawKeyData += std::format("doubleSided={}|", parameters.doubleSided);

        // Include texture references
        if (textures.baseColorTexture)
            rawKeyData += std::format("baseColorTex={}|", textures.baseColorTexture->assetId);
        if (textures.metallicRoughnessTexture)
            rawKeyData += std::format("metallicRoughnessTex={}|", textures.metallicRoughnessTexture->assetId);
        if (textures.normalTexture)
            rawKeyData += std::format("normalTex={}|", textures.normalTexture->assetId);
        if (textures.occlusionTexture)
            rawKeyData += std::format("occlusionTex={}|", textures.occlusionTexture->assetId);
        if (textures.emissiveTexture)
            rawKeyData += std::format("emissiveTex={}|", textures.emissiveTexture->assetId);

        return core::computeStringHash(rawKeyData);
    }
}
