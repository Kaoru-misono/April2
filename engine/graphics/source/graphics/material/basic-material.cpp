#include "basic-material.hpp"

#include "material-system.hpp"
#include "rhi/render-device.hpp"

namespace april::graphics
{
    namespace
    {
        static_assert((sizeof(generated::MaterialHeader) + sizeof(generated::BasicMaterialData)) <= sizeof(generated::MaterialDataBlob));
    }

    BasicMaterial::BasicMaterial(core::ref<Device> p_device, std::string const& name, generated::MaterialType type)
        : Material(std::move(p_device), name, type)
    {
        m_header.setIsBasicMaterial(true);
        m_header.setIoR(1.5f);

        m_textureSlotInfo[static_cast<size_t>(TextureSlot::BaseColor)] = TextureSlotInfo{"baseColor", TextureChannelFlags::Red | TextureChannelFlags::Green | TextureChannelFlags::Blue | TextureChannelFlags::Alpha, true};
        m_textureSlotInfo[static_cast<size_t>(TextureSlot::Specular)] = TextureSlotInfo{"specular", TextureChannelFlags::Red | TextureChannelFlags::Green | TextureChannelFlags::Blue, false};
        m_textureSlotInfo[static_cast<size_t>(TextureSlot::Emissive)] = TextureSlotInfo{"emissive", TextureChannelFlags::Red | TextureChannelFlags::Green | TextureChannelFlags::Blue, true};
        m_textureSlotInfo[static_cast<size_t>(TextureSlot::Normal)] = TextureSlotInfo{"normal", TextureChannelFlags::Red | TextureChannelFlags::Green, false};
        m_textureSlotInfo[static_cast<size_t>(TextureSlot::Transmission)] = TextureSlotInfo{"transmission", TextureChannelFlags::Red, false};
        m_textureSlotInfo[static_cast<size_t>(TextureSlot::Displacement)] = TextureSlotInfo{"displacement", TextureChannelFlags::Red, false};

        updateAlphaMode();
        updateNormalMapType();
        updateEmissiveFlag();
        updateDeltaSpecularFlag();
    }

    auto BasicMaterial::update(MaterialSystem* p_owner) -> MaterialUpdateFlags
    {
        AP_ASSERT(p_owner);

        if (m_updates != UpdateFlags::None)
        {
            adjustDoubleSidedFlag();
            prepareDisplacementMapForRendering();

            updateTextureHandle(p_owner, TextureSlot::BaseColor, m_data.texBaseColor);
            updateTextureHandle(p_owner, TextureSlot::Specular, m_data.texSpecular);
            updateTextureHandle(p_owner, TextureSlot::Emissive, m_data.texEmissive);
            updateTextureHandle(p_owner, TextureSlot::Transmission, m_data.texTransmission);
            updateTextureHandle(p_owner, TextureSlot::Normal, m_data.texNormalMap);
            updateTextureHandle(p_owner, TextureSlot::Displacement, m_data.texDisplacementMap);

            updateDefaultTextureSamplerID(p_owner, m_pDefaultSampler);

            auto prevFlags = m_data.flags;
            m_data.setDisplacementMinSamplerID(p_owner->registerSamplerDescriptor(m_pDisplacementMinSampler));
            m_data.setDisplacementMaxSamplerID(p_owner->registerSamplerDescriptor(m_pDisplacementMaxSampler));
            if (m_data.flags != prevFlags)
            {
                m_updates |= UpdateFlags::DataChanged;
            }

            if (m_updates != UpdateFlags::None && isEmissive())
            {
                m_updates |= UpdateFlags::EmissiveChanged;
            }
        }

        auto const flags = m_updates;
        m_updates = UpdateFlags::None;
        return flags;
    }

    auto BasicMaterial::isDisplaced() const -> bool
    {
        return hasTextureSlotData(TextureSlot::Displacement);
    }

    auto BasicMaterial::isEqual(core::ref<Material> const& p_other) const -> bool
    {
        auto other = core::dynamic_ref_cast<BasicMaterial>(p_other);
        return other && (*this == *other);
    }

    auto BasicMaterial::setAlphaMode(generated::AlphaMode alphaMode) -> void
    {
        if (!isAlphaSupported())
        {
            AP_WARN("Alpha is not supported by material type '{}'. Ignoring setAlphaMode() for material '{}'.", static_cast<uint32_t>(getType()), getName());
            return;
        }

        if (m_header.getAlphaMode() != alphaMode)
        {
            m_header.setAlphaMode(alphaMode);
            markUpdates(UpdateFlags::DataChanged);
        }
    }

    auto BasicMaterial::setAlphaThreshold(float alphaThreshold) -> void
    {
        if (!isAlphaSupported())
        {
            AP_WARN("Alpha is not supported by material type '{}'. Ignoring setAlphaThreshold() for material '{}'.", static_cast<uint32_t>(getType()), getName());
            return;
        }

        if (m_header.getAlphaThreshold() != alphaThreshold)
        {
            m_header.setAlphaThreshold(alphaThreshold);
            markUpdates(UpdateFlags::DataChanged);
            updateAlphaMode();
        }
    }

    auto BasicMaterial::setTexture(TextureSlot const slot, core::ref<Texture> const& p_texture) -> bool
    {
        if (!Material::setTexture(slot, p_texture))
        {
            return false;
        }

        switch (slot)
        {
        case TextureSlot::BaseColor:
            if (p_texture)
            {
                m_alphaRange = float2(0.f, 1.f);
                m_isTexturedBaseColorConstant = false;
                m_isTexturedAlphaConstant = false;
            }
            updateAlphaMode();
            updateDeltaSpecularFlag();
            break;
        case TextureSlot::Specular:
            updateDeltaSpecularFlag();
            break;
        case TextureSlot::Normal:
            updateNormalMapType();
            break;
        case TextureSlot::Emissive:
            updateEmissiveFlag();
            break;
        case TextureSlot::Displacement:
            m_displacementMapChanged = true;
            markUpdates(UpdateFlags::DisplacementChanged);
            break;
        default:
            break;
        }

        return true;
    }

    auto BasicMaterial::optimizeTexture(TextureSlot const slot, TextureOptimizationStats& stats) -> void
    {
        (void)stats;
        if (slot == TextureSlot::Normal)
        {
            updateNormalMapType();
        }
    }

    auto BasicMaterial::setDefaultTextureSampler(core::ref<Sampler> const& p_sampler) -> void
    {
        if (p_sampler == m_pDefaultSampler)
        {
            return;
        }

        m_pDefaultSampler = p_sampler;

        if (p_sampler && m_device)
        {
            auto desc = p_sampler->getDesc();
            desc.setMaxAnisotropy(16);
            desc.setReductionMode(TextureReductionMode::Min);
            m_pDisplacementMinSampler = m_device->createSampler(desc);

            desc.setReductionMode(TextureReductionMode::Max);
            m_pDisplacementMaxSampler = m_device->createSampler(desc);
        }

        markUpdates(UpdateFlags::ResourcesChanged);
    }

    auto BasicMaterial::setBaseColor(float4 const& color) -> void
    {
        auto const current = float4(m_data.baseColor);
        if (current != color)
        {
            m_data.baseColor = generated::float16_t4(color);
            markUpdates(UpdateFlags::DataChanged);
            updateAlphaMode();
            updateDeltaSpecularFlag();
        }
    }

    auto BasicMaterial::setSpecularParams(float4 const& value) -> void
    {
        auto const current = float4(m_data.specular);
        if (current != value)
        {
            m_data.specular = generated::float16_t4(value);
            markUpdates(UpdateFlags::DataChanged);
            updateDeltaSpecularFlag();
        }
    }

    auto BasicMaterial::setTransmissionColor(float3 const& value) -> void
    {
        auto const current = float3(m_data.transmission);
        if (current != value)
        {
            m_data.transmission = generated::float16_t3(value);
            markUpdates(UpdateFlags::DataChanged);
        }
    }

    auto BasicMaterial::setDiffuseTransmission(float value) -> void
    {
        if (float(m_data.diffuseTransmission) != value)
        {
            m_data.diffuseTransmission = generated::float16_t(value);
            markUpdates(UpdateFlags::DataChanged);
            updateDeltaSpecularFlag();
        }
    }

    auto BasicMaterial::setSpecularTransmission(float value) -> void
    {
        if (float(m_data.specularTransmission) != value)
        {
            m_data.specularTransmission = generated::float16_t(value);
            markUpdates(UpdateFlags::DataChanged);
            updateDeltaSpecularFlag();
        }
    }

    auto BasicMaterial::setVolumeAbsorption(float3 const& volumeAbsorption) -> void
    {
        auto const current = float3(m_data.volumeAbsorption);
        if (current != volumeAbsorption)
        {
            m_data.volumeAbsorption = generated::float16_t3(volumeAbsorption);
            markUpdates(UpdateFlags::DataChanged);
        }
    }

    auto BasicMaterial::setVolumeScattering(float3 const& volumeScattering) -> void
    {
        auto const current = float3(m_data.volumeScattering);
        if (current != volumeScattering)
        {
            m_data.volumeScattering = generated::float16_t3(volumeScattering);
            markUpdates(UpdateFlags::DataChanged);
        }
    }

    auto BasicMaterial::setVolumeAnisotropy(float volumeAnisotropy) -> void
    {
        auto const clamped = glm::clamp(volumeAnisotropy, -0.99f, 0.99f);
        if (float(m_data.volumeAnisotropy) != clamped)
        {
            m_data.volumeAnisotropy = generated::float16_t(clamped);
            markUpdates(UpdateFlags::DataChanged);
        }
    }

    auto BasicMaterial::setDisplacementScale(float value) -> void
    {
        if (m_data.displacementScale != value)
        {
            m_data.displacementScale = value;
            markUpdates(UpdateFlags::DataChanged | UpdateFlags::DisplacementChanged);
        }
    }

    auto BasicMaterial::setDisplacementOffset(float value) -> void
    {
        if (m_data.displacementOffset != value)
        {
            m_data.displacementOffset = value;
            markUpdates(UpdateFlags::DataChanged | UpdateFlags::DisplacementChanged);
        }
    }

    auto BasicMaterial::operator==(BasicMaterial const& other) const -> bool
    {
        return isBaseEqual(other)
            && m_data.flags == other.m_data.flags
            && m_data.emissiveFactor == other.m_data.emissiveFactor
            && float4(m_data.baseColor) == float4(other.m_data.baseColor)
            && float4(m_data.specular) == float4(other.m_data.specular)
            && float3(m_data.emissive) == float3(other.m_data.emissive)
            && float(m_data.specularTransmission) == float(other.m_data.specularTransmission)
            && float3(m_data.transmission) == float3(other.m_data.transmission)
            && float(m_data.diffuseTransmission) == float(other.m_data.diffuseTransmission)
            && m_data.displacementScale == other.m_data.displacementScale
            && m_data.displacementOffset == other.m_data.displacementOffset;
    }

    auto BasicMaterial::isAlphaSupported() const -> bool
    {
        return enum_has_all_flags(getTextureSlotInfo(TextureSlot::BaseColor).mask, TextureChannelFlags::Alpha);
    }

    auto BasicMaterial::prepareDisplacementMapForRendering() -> void
    {
        m_displacementMapChanged = false;
    }

    auto BasicMaterial::adjustDoubleSidedFlag() -> void
    {
        auto doubleSided = Material::isDoubleSided();
        if (getDiffuseTransmission() > 0.f || getSpecularTransmission() > 0.f)
        {
            doubleSided = true;
        }
        if (isDisplaced())
        {
            doubleSided = true;
        }
        Material::setDoubleSided(doubleSided);
    }

    auto BasicMaterial::updateAlphaMode() -> void
    {
        if (!isAlphaSupported())
        {
            return;
        }

        auto const alpha = getBaseColor().w;
        if (!getBaseColorTexture())
        {
            m_alphaRange = float2(alpha);
        }

        auto const useAlpha = m_alphaRange.x < getAlphaThreshold();
        Material::setAlphaMode(useAlpha ? generated::AlphaMode::Mask : generated::AlphaMode::Opaque);
    }

    auto BasicMaterial::updateNormalMapType() -> void
    {
        auto const type = detectNormalMapType(getNormalMap());
        if (m_data.getNormalMapType() != type)
        {
            m_data.setNormalMapType(type);
            markUpdates(UpdateFlags::DataChanged);
        }
    }

    auto BasicMaterial::updateEmissiveFlag() -> void
    {
        auto emissive = false;
        if (m_data.emissiveFactor > 0.0f)
        {
            auto const e = float3(m_data.emissive);
            emissive = hasTextureSlotData(TextureSlot::Emissive) || e.x != 0.0f || e.y != 0.0f || e.z != 0.0f;
        }

        if (m_header.isEmissive() != emissive)
        {
            m_header.setEmissive(emissive);
            markUpdates(UpdateFlags::DataChanged | UpdateFlags::EmissiveChanged);
        }
    }
}
