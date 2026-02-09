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

auto StandardMaterial::writeData(generated::StandardMaterialData& data) const -> void
{
    // Header
    data.header.abiVersion = 1;
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
    m_doubleSided = doubleSided;
}

auto StandardMaterial::isDoubleSided() const -> bool
{
    return m_doubleSided;
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
