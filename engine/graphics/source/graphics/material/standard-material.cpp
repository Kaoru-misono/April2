// StandardMaterial implementation.

#include "standard-material.hpp"
#include "program/shader-variable.hpp"

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
    material->m_doubleSided = params.doubleSided;

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
    // Header
    data.header.abiVersion = generated::kMaterialAbiVersion;
    data.header.materialType = static_cast<uint32_t>(generated::MaterialType::Standard);
    data.header.flags = updateFlags();
    data.header.alphaMode = static_cast<uint32_t>(alphaMode);
    data.header.activeLobes = updateActiveLobes();
    data.header.reserved0 = 0;
    data.header.reserved1 = 0;
    data.header.reserved2 = 0;

    // Material parameters
    data.baseColor = baseColor;
    data.metallic = metallic;
    data.roughness = roughness;
    data.ior = ior;
    data.specularTransmission = specularTransmission;
    data.emissive = emissive;
    data.diffuseTransmission = diffuseTransmission;
    data.transmission = transmission;
    data.alphaCutoff = alphaCutoff;
    data.normalScale = normalScale;
    data.occlusionStrength = occlusionStrength;
    data._padding = float2{0.0f, 0.0f};
    data.baseColorTextureHandle = m_baseColorTextureHandle;
    data.metallicRoughnessTextureHandle = m_metallicRoughnessTextureHandle;
    data.normalTextureHandle = m_normalTextureHandle;
    data.occlusionTextureHandle = m_occlusionTextureHandle;
    data.emissiveTextureHandle = m_emissiveTextureHandle;
    data.samplerHandle = m_samplerHandle;
    data.bufferHandle = m_bufferHandle;
    data.reservedDescriptorHandle = 0;
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

auto StandardMaterial::bindTextures(ShaderVariable& var) const -> void
{
    // Bind textures to shader variable if they exist
    if (baseColorTexture)
    {
        if (var.hasMember("baseColorTexture"))
            var["baseColorTexture"].setTexture(baseColorTexture);
    }

    if (metallicRoughnessTexture)
    {
        if (var.hasMember("metallicRoughnessTexture"))
            var["metallicRoughnessTexture"].setTexture(metallicRoughnessTexture);
    }

    if (normalTexture)
    {
        if (var.hasMember("normalTexture"))
            var["normalTexture"].setTexture(normalTexture);
    }

    if (occlusionTexture)
    {
        if (var.hasMember("occlusionTexture"))
            var["occlusionTexture"].setTexture(occlusionTexture);
    }

    if (emissiveTexture)
    {
        if (var.hasMember("emissiveTexture"))
            var["emissiveTexture"].setTexture(emissiveTexture);
    }
}

auto StandardMaterial::hasTextures() const -> bool
{
    return baseColorTexture != nullptr ||
           metallicRoughnessTexture != nullptr ||
           normalTexture != nullptr ||
           occlusionTexture != nullptr ||
           emissiveTexture != nullptr;
}

auto StandardMaterial::getFlags() const -> uint32_t
{
    return updateFlags();
}

auto StandardMaterial::setDoubleSided(bool doubleSided) -> void
{
    if (m_doubleSided != doubleSided)
    {
        m_doubleSided = doubleSided;
        markDirty(MaterialUpdateFlags::DataChanged);
    }
}

auto StandardMaterial::isDoubleSided() const -> bool
{
    return m_doubleSided;
}

auto StandardMaterial::serializeParameters(nlohmann::json& outJson) const -> void
{
    outJson["type"] = "Standard";
    outJson["baseColor"] = baseColor;
    outJson["metallic"] = metallic;
    outJson["roughness"] = roughness;
    outJson["ior"] = ior;
    outJson["specularTransmission"] = specularTransmission;
    outJson["emissive"] = emissive;
    outJson["diffuseTransmission"] = diffuseTransmission;
    outJson["transmission"] = transmission;
    outJson["alphaCutoff"] = alphaCutoff;
    outJson["normalScale"] = normalScale;
    outJson["occlusionStrength"] = occlusionStrength;
    outJson["doubleSided"] = m_doubleSided;

    // Alpha mode
    switch (alphaMode)
    {
    case generated::AlphaMode::Opaque: outJson["alphaMode"] = "OPAQUE"; break;
    case generated::AlphaMode::Mask: outJson["alphaMode"] = "MASK"; break;
    case generated::AlphaMode::Blend: outJson["alphaMode"] = "BLEND"; break;
    }

    // Diffuse model
    switch (diffuseModel)
    {
    case DiffuseBRDFModel::Lambert: outJson["diffuseModel"] = "Lambert"; break;
    case DiffuseBRDFModel::Frostbite: outJson["diffuseModel"] = "Frostbite"; break;
    }
}

auto StandardMaterial::deserializeParameters(nlohmann::json const& inJson) -> bool
{
    if (inJson.contains("baseColor")) baseColor = inJson["baseColor"].get<float4>();
    if (inJson.contains("metallic")) metallic = inJson["metallic"].get<float>();
    if (inJson.contains("roughness")) roughness = inJson["roughness"].get<float>();
    if (inJson.contains("ior")) ior = inJson["ior"].get<float>();
    if (inJson.contains("specularTransmission")) specularTransmission = inJson["specularTransmission"].get<float>();
    if (inJson.contains("emissive")) emissive = inJson["emissive"].get<float3>();
    if (inJson.contains("diffuseTransmission")) diffuseTransmission = inJson["diffuseTransmission"].get<float>();
    if (inJson.contains("transmission")) transmission = inJson["transmission"].get<float3>();
    if (inJson.contains("alphaCutoff")) alphaCutoff = inJson["alphaCutoff"].get<float>();
    if (inJson.contains("normalScale")) normalScale = inJson["normalScale"].get<float>();
    if (inJson.contains("occlusionStrength")) occlusionStrength = inJson["occlusionStrength"].get<float>();
    if (inJson.contains("doubleSided")) m_doubleSided = inJson["doubleSided"].get<bool>();

    if (inJson.contains("alphaMode"))
    {
        auto const mode = inJson["alphaMode"].get<std::string>();
        if (mode == "OPAQUE") alphaMode = generated::AlphaMode::Opaque;
        else if (mode == "MASK") alphaMode = generated::AlphaMode::Mask;
        else if (mode == "BLEND") alphaMode = generated::AlphaMode::Blend;
    }

    if (inJson.contains("diffuseModel"))
    {
        auto const model = inJson["diffuseModel"].get<std::string>();
        if (model == "Lambert") diffuseModel = DiffuseBRDFModel::Lambert;
        else if (model == "Frostbite") diffuseModel = DiffuseBRDFModel::Frostbite;
    }

    markDirty(MaterialUpdateFlags::All);
    return true;
}

auto StandardMaterial::setDescriptorHandles(
    uint32_t baseColorTextureHandle,
    uint32_t metallicRoughnessTextureHandle,
    uint32_t normalTextureHandle,
    uint32_t occlusionTextureHandle,
    uint32_t emissiveTextureHandle,
    uint32_t samplerHandle,
    uint32_t bufferHandle
) -> void
{
    m_baseColorTextureHandle = baseColorTextureHandle;
    m_metallicRoughnessTextureHandle = metallicRoughnessTextureHandle;
    m_normalTextureHandle = normalTextureHandle;
    m_occlusionTextureHandle = occlusionTextureHandle;
    m_emissiveTextureHandle = emissiveTextureHandle;
    m_samplerHandle = samplerHandle;
    m_bufferHandle = bufferHandle;
}

auto StandardMaterial::updateFlags() const -> uint32_t
{
    uint32_t flags = static_cast<uint32_t>(generated::MaterialFlags::None);

    if (m_doubleSided)
        flags |= static_cast<uint32_t>(generated::MaterialFlags::DoubleSided);

    if (alphaMode == generated::AlphaMode::Mask)
        flags |= static_cast<uint32_t>(generated::MaterialFlags::AlphaTested);

    if (emissive.x > 0.0f || emissive.y > 0.0f || emissive.z > 0.0f)
        flags |= static_cast<uint32_t>(generated::MaterialFlags::Emissive);

    if (hasTextures())
        flags |= static_cast<uint32_t>(generated::MaterialFlags::HasTextures);

    return flags;
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
