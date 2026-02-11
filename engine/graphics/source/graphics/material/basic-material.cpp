#include "basic-material.hpp"

#include "material-system.hpp"
#include "rhi/command-context.hpp"
#include "rhi/render-device.hpp"

namespace april::graphics
{
    namespace
    {
        static_assert((sizeof(generated::MaterialHeader) + sizeof(generated::BasicMaterialData)) <= sizeof(generated::MaterialDataBlob));
        static_assert(static_cast<uint32_t>(generated::ShadingModel::Count) <= (1u << generated::BasicMaterialData::kShadingModelBits));
        static_assert(static_cast<uint32_t>(generated::NormalMapType::Count) <= (1u << generated::BasicMaterialData::kNormalMapTypeBits));
        static_assert(generated::BasicMaterialData::kTotalFlagsBits <= 32);

        auto constexpr kMaxVolumeAnisotropy = 0.99f;
    }

    BasicMaterial::BasicMaterial(core::ref<Device> p_device, std::string const& name, generated::MaterialType type)
        : Material(std::move(p_device), name, type)
    {
        m_header.setIsBasicMaterial(true);
        m_header.setIoR(1.5f);

        m_textureSlotInfo[static_cast<size_t>(TextureSlot::Displacement)] = TextureSlotInfo{"displacement", TextureChannelFlags::RGB, false};

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
            m_data.setDisplacementMinSamplerID(p_owner->addTextureSampler(m_pDisplacementMinSampler));
            m_data.setDisplacementMaxSamplerID(p_owner->addTextureSampler(m_pDisplacementMaxSampler));
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

    auto BasicMaterial::setAlphaMode(generated::AlphaMode alphaMode) -> void
    {
        if (!isAlphaSupported())
        {
            AP_ASSERT(getAlphaMode() == generated::AlphaMode::Opaque);
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

        // TODO(falcor-align): Compare against float16-cast threshold like Falcor (mHeader.getAlphaThreshold() != (float16_t)alphaThreshold).
        if (m_header.getAlphaThreshold() != alphaThreshold)
        {
            m_header.setAlphaThreshold(alphaThreshold);
            markUpdates(UpdateFlags::DataChanged);
            updateAlphaMode();
        }
    }

    auto BasicMaterial::setDefaultTextureSampler(core::ref<Sampler> const& p_sampler) -> void
    {
        if (p_sampler != m_pDefaultSampler)
        {
            m_pDefaultSampler = p_sampler;

            auto desc = p_sampler->getDesc();
            desc.setMaxAnisotropy(16);
            desc.setReductionMode(TextureReductionMode::Min);
            m_pDisplacementMinSampler = m_device->createSampler(desc);
            desc.setReductionMode(TextureReductionMode::Max);
            m_pDisplacementMaxSampler = m_device->createSampler(desc);

            markUpdates(UpdateFlags::ResourcesChanged);
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

    auto BasicMaterial::optimizeTexture(TextureSlot const slot, TextureAnalyzer::Result const& texInfo, TextureOptimizationStats& stats) -> void
    {
        AP_ASSERT(getTexture(slot) != nullptr);
        auto channelMask = getTextureSlotInfo(slot).mask;

        switch (slot)
        {
        case TextureSlot::BaseColor:
        {
            auto const previouslyOpaque = isOpaque();

            auto const pBaseColor = getBaseColorTexture();
            auto const hasAlpha = isAlphaSupported() && pBaseColor && doesFormatHaveAlpha(pBaseColor->getFormat());
            auto const isColorConstant = texInfo.isConstant(TextureChannelFlags::RGB);
            auto const isAlphaConstant = texInfo.isConstant(TextureChannelFlags::Alpha);

            if (hasAlpha)
            {
                m_alphaRange = float2(texInfo.minValue.w, texInfo.maxValue.w);
            }

            auto baseColor = getBaseColor();
            if (isColorConstant)
            {
                baseColor = float4(texInfo.value.x, texInfo.value.y, texInfo.value.z, baseColor.w);
                m_isTexturedBaseColorConstant = true;
            }

            if (hasAlpha && isAlphaConstant)
            {
                baseColor = float4(baseColor.x, baseColor.y, baseColor.z, texInfo.value.w);
                m_isTexturedAlphaConstant = true;
            }
            setBaseColor(baseColor);

            if (isColorConstant && (!hasAlpha || isAlphaConstant))
            {
                clearTexture(TextureSlot::BaseColor);
                ++stats.texturesRemoved[static_cast<size_t>(slot)];
            }
            else if (isColorConstant)
            {
                ++stats.constantBaseColor;
            }

            updateAlphaMode();
            if (!previouslyOpaque && isOpaque())
            {
                ++stats.disabledAlpha;
            }
            break;
        }
        case TextureSlot::Specular:
            if (texInfo.isConstant(channelMask))
            {
                clearTexture(TextureSlot::Specular);
                setSpecularParams(texInfo.value);
                ++stats.texturesRemoved[static_cast<size_t>(slot)];
            }
            break;
        case TextureSlot::Emissive:
            if (texInfo.isConstant(channelMask))
            {
                clearTexture(TextureSlot::Emissive);
                setEmissiveColor(float3(texInfo.value.x, texInfo.value.y, texInfo.value.z));
                ++stats.texturesRemoved[static_cast<size_t>(slot)];
            }
            break;
        case TextureSlot::Normal:
            switch (getNormalMapType())
            {
            case generated::NormalMapType::RG:
                channelMask = TextureChannelFlags::Red | TextureChannelFlags::Green;
                break;
            case generated::NormalMapType::RGB:
                channelMask = TextureChannelFlags::RGB;
                break;
            default:
                AP_WARN("BasicMaterial::optimizeTexture() - Unsupported normal map mode");
                channelMask = TextureChannelFlags::RGBA;
                break;
            }

            if (texInfo.isConstant(channelMask))
            {
                ++stats.constantNormalMaps;
            }
            break;
        case TextureSlot::Transmission:
            if (texInfo.isConstant(channelMask))
            {
                clearTexture(TextureSlot::Transmission);
                setTransmissionColor(float3(texInfo.value.x, texInfo.value.y, texInfo.value.z));
                ++stats.texturesRemoved[static_cast<size_t>(slot)];
            }
            break;
        case TextureSlot::Displacement:
            break;
        default:
            // TODO(falcor-align): Falcor throws on unexpected slot here; map to equivalent throw path instead of AP_UNREACHABLE().
            AP_UNREACHABLE();
            break;
        }
    }

    auto BasicMaterial::isAlphaSupported() const -> bool
    {
        return enum_has_all_flags(getTextureSlotInfo(TextureSlot::BaseColor).mask, TextureChannelFlags::Alpha);
    }

    auto BasicMaterial::prepareDisplacementMapForRendering() -> void
    {
        if (auto p_displacementMap = getDisplacementMap(); p_displacementMap && m_displacementMapChanged)
        {
            auto* p_context = m_device ? m_device->getCommandContext() : nullptr;

            if (p_context && getFormatChannelCount(p_displacementMap->getFormat()) < 4)
            {
                auto const usage = p_displacementMap->getUsage() | TextureUsage::UnorderedAccess | TextureUsage::RenderTarget;
                auto p_newDisplacementMap = m_device->createTexture2D(
                    p_displacementMap->getWidth(),
                    p_displacementMap->getHeight(),
                    ResourceFormat::RGBA16Float,
                    p_displacementMap->getArraySize(),
                    Resource::kMaxPossible,
                    nullptr,
                    usage
                );

                for (auto arraySlice = uint32_t{0}; arraySlice < p_displacementMap->getArraySize(); ++arraySlice)
                {
                    auto p_srv = p_displacementMap->getSRV(0, 1, arraySlice, 1);
                    auto p_rtv = p_newDisplacementMap->getRTV(0, arraySlice, 1);
                    auto colorTargets = ColorTargets{};
                    colorTargets.emplace_back(p_rtv, LoadOp::DontCare, StoreOp::Store);

                    if (auto p_renderPass = p_context->beginRenderPass(colorTargets))
                    {
                        // TODO(falcor-align): Falcor uses RenderContext::blit() with reduction modes and componentsTransform for displacement packing.
                        p_renderPass->blit(p_srv, p_rtv, RenderPassEncoder::kMaxRect, RenderPassEncoder::kMaxRect, TextureFilteringMode::Linear);
                        p_renderPass->end();
                    }
                }

                p_displacementMap = p_newDisplacementMap;
                setDisplacementMap(p_newDisplacementMap);
            }

            p_displacementMap->generateMips(p_context, true);
        }

        m_displacementMapChanged = false;
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

    auto BasicMaterial::setBaseColor(float4 const& color) -> void
    {
        // TODO(falcor-align): Use Falcor-style vector compare/cast path (any(mData.baseColor != (float16_t4)color)).
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
        // TODO(falcor-align): Use Falcor-style vector compare/cast path (any(mData.specular != (float16_t4)value)).
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
        // TODO(falcor-align): Use Falcor-style vector compare/cast path (any(mData.transmission != (float16_t3)value)).
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
        // TODO(falcor-align): Use Falcor-style vector compare/cast path (any(mData.volumeAbsorption != (float16_t3)volumeAbsorption)).
        auto const current = float3(m_data.volumeAbsorption);
        if (current != volumeAbsorption)
        {
            m_data.volumeAbsorption = generated::float16_t3(volumeAbsorption);
            markUpdates(UpdateFlags::DataChanged);
        }
    }

    auto BasicMaterial::setVolumeScattering(float3 const& volumeScattering) -> void
    {
        // TODO(falcor-align): Use Falcor-style vector compare/cast path (any(mData.volumeScattering != (float16_t3)volumeScattering)).
        auto const current = float3(m_data.volumeScattering);
        if (current != volumeScattering)
        {
            m_data.volumeScattering = generated::float16_t3(volumeScattering);
            markUpdates(UpdateFlags::DataChanged);
        }
    }

    auto BasicMaterial::setVolumeAnisotropy(float volumeAnisotropy) -> void
    {
        auto const clamped = glm::clamp(volumeAnisotropy, -kMaxVolumeAnisotropy, kMaxVolumeAnisotropy);
        if (float(m_data.volumeAnisotropy) != clamped)
        {
            m_data.volumeAnisotropy = generated::float16_t(clamped);
            markUpdates(UpdateFlags::DataChanged);
        }
    }

    auto BasicMaterial::isEqual(core::ref<Material> const& p_other) const -> bool
    {
        auto const other = core::dynamic_ref_cast<BasicMaterial>(p_other);
        if (!other)
        {
            return false;
        }

        return *this == *other;
    }

    auto BasicMaterial::operator==(BasicMaterial const& other) const -> bool
    {
        // TODO(falcor-align): Match Falcor compare_vec_field macros exactly (compare half-vector storage directly via any()).
        if (!isBaseEqual(other))
        {
            return false;
        }

#define compare_field(field_) if (m_data.field_ != other.m_data.field_) return false
#define compare_vec_field(field_) if (float3(m_data.field_) != float3(other.m_data.field_)) return false
        compare_field(flags);
        compare_field(displacementScale);
        compare_field(displacementOffset);
        if (float4(m_data.baseColor) != float4(other.m_data.baseColor)) return false;
        if (float4(m_data.specular) != float4(other.m_data.specular)) return false;
        compare_vec_field(emissive);
        compare_field(emissiveFactor);
        compare_field(diffuseTransmission);
        compare_field(specularTransmission);
        compare_vec_field(transmission);
        compare_vec_field(volumeAbsorption);
        compare_field(volumeAnisotropy);
        compare_vec_field(volumeScattering);
#undef compare_field
#undef compare_vec_field

        if (m_pDefaultSampler->getDesc() != other.m_pDefaultSampler->getDesc())
        {
            return false;
        }

        if (m_pDisplacementMinSampler->getDesc() != other.m_pDisplacementMinSampler->getDesc())
        {
            return false;
        }

        if (m_pDisplacementMaxSampler->getDesc() != other.m_pDisplacementMaxSampler->getDesc())
        {
            return false;
        }

        return true;
    }

    auto BasicMaterial::updateAlphaMode() -> void
    {
        if (!isAlphaSupported())
        {
            AP_ASSERT(getAlphaMode() == generated::AlphaMode::Opaque);
            return;
        }

        auto const hasAlpha = getBaseColorTexture() && doesFormatHaveAlpha(getBaseColorTexture()->getFormat());
        auto const alpha = getBaseColor().w;
        if (!hasAlpha)
        {
            m_alphaRange = float2(alpha);
        }

        auto const useAlpha = m_alphaRange.x < getAlphaThreshold();
        setAlphaMode(useAlpha ? generated::AlphaMode::Mask : generated::AlphaMode::Opaque);
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
}
