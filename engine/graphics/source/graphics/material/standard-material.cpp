// StandardMaterial implementation.

#include "standard-material.hpp"

namespace april::graphics
{

StandardMaterial::StandardMaterial()
{
    m_activeLobes = updateActiveLobes();
}

auto StandardMaterial::createFromAsset(
    core::ref<Device> device,
    asset::MaterialAsset const& asset
) -> core::ref<StandardMaterial>
{
    auto material = core::make_ref<StandardMaterial>();

    // Copy parameters from asset
    auto const& params = asset.parameters;
    material->baseColor = params.baseColorFactor;
    material->metallic = params.metallicFactor;
    material->roughness = params.roughnessFactor;
    material->emissive = params.emissiveFactor;
    material->occlusionStrength = params.occlusionStrength;
    material->normalScale = params.normalScale;
    material->alphaCutoff = params.alphaCutoff;
    material->setDoubleSided(params.doubleSided);

    // Parse alpha mode
    if (params.alphaMode == "OPAQUE")
        material->alphaMode = generated::AlphaMode::Opaque;
    else if (params.alphaMode == "MASK")
        material->alphaMode = generated::AlphaMode::Mask;
    else if (params.alphaMode == "BLEND")
        material->alphaMode = generated::AlphaMode::Blend;

    // Textures would be loaded from the asset's texture references here
    // For now, we leave them as null and let the caller set them

    return material;
}

auto StandardMaterial::getType() const -> generated::MaterialType
{
    return generated::MaterialType::Standard;
}

auto StandardMaterial::getTypeName() const -> std::string
{
    return "Standard";
}

auto StandardMaterial::writeData(generated::StandardMaterialData& data) const -> void
{
    writeCommonData(data);
    data.header.materialType = static_cast<uint32_t>(generated::MaterialType::Standard);
    data.header.activeLobes = updateActiveLobes();

    data.metallic = metallic;
    data.roughness = roughness;
    data.normalScale = normalScale;
    data.occlusionStrength = occlusionStrength;
    data._padding = float2{0.0f, 0.0f};
}

auto StandardMaterial::getTypeConformances() const -> TypeConformanceList
{
    TypeConformanceList conformances;

    // Select diffuse BRDF implementation based on model
    switch (diffuseModel)
    {
    case DiffuseBRDFModel::Lambert:
        conformances.add("LambertDiffuseBRDF", "IDiffuseBRDF");
        break;
    case DiffuseBRDFModel::Frostbite:
    default:
        conformances.add("FrostbiteDiffuseBRDF", "IDiffuseBRDF");
        break;
    }

    conformances.add("StandardMaterialInstance", "IMaterialInstance");

    return conformances;
}

auto StandardMaterial::serializeParameters(nlohmann::json& outJson) const -> void
{
    outJson["type"] = "Standard";
    serializeCommonParameters(outJson);
    outJson["metallic"] = metallic;
    outJson["roughness"] = roughness;
    outJson["normalScale"] = normalScale;
    outJson["occlusionStrength"] = occlusionStrength;

    // Diffuse model
    switch (diffuseModel)
    {
    case DiffuseBRDFModel::Lambert: outJson["diffuseModel"] = "Lambert"; break;
    case DiffuseBRDFModel::Frostbite: outJson["diffuseModel"] = "Frostbite"; break;
    }
}

auto StandardMaterial::deserializeParameters(nlohmann::json const& inJson) -> bool
{
    deserializeCommonParameters(inJson);
    if (inJson.contains("metallic")) metallic = inJson["metallic"].get<float>();
    if (inJson.contains("roughness")) roughness = inJson["roughness"].get<float>();
    if (inJson.contains("normalScale")) normalScale = inJson["normalScale"].get<float>();
    if (inJson.contains("occlusionStrength")) occlusionStrength = inJson["occlusionStrength"].get<float>();

    if (inJson.contains("diffuseModel"))
    {
        auto const model = inJson["diffuseModel"].get<std::string>();
        if (model == "Lambert") diffuseModel = DiffuseBRDFModel::Lambert;
        else if (model == "Frostbite") diffuseModel = DiffuseBRDFModel::Frostbite;
    }

    markDirty(MaterialUpdateFlags::All);
    return true;
}

auto StandardMaterial::updateActiveLobes() const -> uint32_t
{
    uint32_t lobes = 0;

    // Check for diffuse
    auto const dielectricContribution = (1.0f - metallic) * (1.0f - specularTransmission);
    if (dielectricContribution > 0.0f)
    {
        if (diffuseTransmission < 1.0f)
            lobes |= static_cast<uint32_t>(generated::LobeType::DiffuseReflection);
        if (diffuseTransmission > 0.0f)
            lobes |= static_cast<uint32_t>(generated::LobeType::DiffuseTransmission);
    }

    // Specular reflection is always active (metals and dielectrics)
    auto const alpha = roughness * roughness;
    constexpr auto kMinGGXAlpha = 0.0064f;
    if (alpha < kMinGGXAlpha)
        lobes |= static_cast<uint32_t>(generated::LobeType::DeltaReflection);
    else
        lobes |= static_cast<uint32_t>(generated::LobeType::SpecularReflection);

    // Specular transmission
    if (specularTransmission > 0.0f)
    {
        if (alpha < kMinGGXAlpha)
            lobes |= static_cast<uint32_t>(generated::LobeType::DeltaTransmission);
        else
            lobes |= static_cast<uint32_t>(generated::LobeType::SpecularTransmission);
    }

    return lobes;
}

} // namespace april::graphics
