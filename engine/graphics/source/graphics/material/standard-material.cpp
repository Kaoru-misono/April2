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

auto StandardMaterial::update(MaterialSystem* pOwner) -> MaterialUpdateFlags
{
    (void)pOwner;
    m_activeLobes = updateActiveLobes();
    return consumeUpdates();
}

auto StandardMaterial::getDataBlob() const -> generated::MaterialDataBlob
{
    generated::BasicMaterialData data{};
    writeCommonData(data);
    data.specular = float4{occlusionStrength, roughness, metallic, 0.0f};
    data.setShadingModel(generated::ShadingModel::MetalRough);

    auto blob = prepareDataBlob(data);
    blob.header.packedData = uint4{0, 0, 0, 0};
    blob.header.setMaterialType(generated::MaterialType::Standard);
    blob.header.setNestedPriority(0);
    blob.header.setActiveLobes(updateActiveLobes());
    blob.header.setDoubleSided(isDoubleSided());
    blob.header.setThinSurface(false);
    blob.header.setEmissive(hasEmissive());
    blob.header.setIsBasicMaterial(true);
    blob.header.setAlphaMode(alphaMode);
    blob.header.setAlphaThreshold(alphaCutoff);
    blob.header.setDefaultTextureSamplerID(getSamplerHandle());
    blob.header.setEnableLightProfile(false);
    blob.header.setIoR(ior);
    blob.header.setAlphaTextureHandle(data.texBaseColor);

    auto const deltaOnlyMask =
        static_cast<uint32_t>(generated::LobeType::DeltaReflection) |
        static_cast<uint32_t>(generated::LobeType::DeltaTransmission);
    auto const activeLobes = updateActiveLobes();
    auto const hasNonDelta = (activeLobes & ~deltaOnlyMask) != 0;
    auto const hasDelta = (activeLobes & deltaOnlyMask) != 0;
    blob.header.setDeltaSpecular(hasDelta && !hasNonDelta);

    return blob;
}

auto StandardMaterial::getTypeConformances() const -> TypeConformanceList
{
    TypeConformanceList conformances;

    conformances.add(
        "StandardMaterial",
        "IMaterial",
        static_cast<uint32_t>(generated::MaterialType::Standard)
    );

    return conformances;
}

auto StandardMaterial::getShaderModules() const -> ProgramDesc::ShaderModuleList
{
    ProgramDesc::ShaderModuleList modules;
    modules.push_back(ProgramDesc::ShaderModule::fromFile("graphics/material/standard-material.slang"));
    modules.push_back(ProgramDesc::ShaderModule::fromFile("graphics/material/material-param-layout.slang"));
    modules.push_back(ProgramDesc::ShaderModule::fromFile("graphics/material/serialized-material-params.slang"));
    modules.push_back(ProgramDesc::ShaderModule::fromFile("graphics/material/phase/i-phase-function.slang"));
    modules.push_back(ProgramDesc::ShaderModule::fromFile("graphics/material/phase/isotropic-phase-function.slang"));
    modules.push_back(ProgramDesc::ShaderModule::fromFile("graphics/material/phase/henyey-greenstein-phase-function.slang"));
    return modules;
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
