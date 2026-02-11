#include "basic-material.hpp"

#include "program/shader-variable.hpp"

#include <array>

namespace april::graphics
{
    namespace
    {
        auto makeTextureHandle(uint32_t descriptorHandle) -> generated::TextureHandle
        {
            auto handle = generated::TextureHandle{};
            if (descriptorHandle != 0)
            {
                handle.setTextureID(descriptorHandle);
            }
            return handle;
        }

        auto kTextureSlotInfos = std::array<IMaterial::TextureSlotInfo, static_cast<size_t>(IMaterial::TextureSlot::Count)>{
            IMaterial::TextureSlotInfo{"baseColor", TextureChannelFlags::Red | TextureChannelFlags::Green | TextureChannelFlags::Blue | TextureChannelFlags::Alpha, true},
            IMaterial::TextureSlotInfo{"specular", TextureChannelFlags::Red | TextureChannelFlags::Green | TextureChannelFlags::Blue, false},
            IMaterial::TextureSlotInfo{"emissive", TextureChannelFlags::Red | TextureChannelFlags::Green | TextureChannelFlags::Blue, true},
            IMaterial::TextureSlotInfo{"normal", TextureChannelFlags::Red | TextureChannelFlags::Green, false},
            IMaterial::TextureSlotInfo{"transmission", TextureChannelFlags::Red, false},
            IMaterial::TextureSlotInfo{"displacement", TextureChannelFlags::Red, false},
        };
    }

    auto BasicMaterial::update(MaterialSystem* pOwner) -> MaterialUpdateFlags
    {
        (void)pOwner;
        return consumeUpdates();
    }

    auto BasicMaterial::getTextureSlotInfo(TextureSlot slot) const -> TextureSlotInfo const&
    {
        auto const slotIndex = static_cast<size_t>(slot);
        if (slotIndex >= kTextureSlotInfos.size())
        {
            return getDisabledTextureSlotInfo();
        }
        return kTextureSlotInfos[slotIndex];
    }

    auto BasicMaterial::setTexture(TextureSlot slot, core::ref<Texture> texture) -> bool
    {
        switch (slot)
        {
        case TextureSlot::BaseColor:
            baseColorTexture = std::move(texture);
            break;
        case TextureSlot::Specular:
            metallicRoughnessTexture = std::move(texture);
            break;
        case TextureSlot::Emissive:
            emissiveTexture = std::move(texture);
            break;
        case TextureSlot::Normal:
            normalTexture = std::move(texture);
            break;
        case TextureSlot::Transmission:
            transmissionTexture = std::move(texture);
            break;
        case TextureSlot::Displacement:
            displacementTexture = std::move(texture);
            break;
        default:
            return false;
        }

        markUpdates(MaterialUpdateFlags::ResourcesChanged);
        return true;
    }

    auto BasicMaterial::getTexture(TextureSlot slot) const -> core::ref<Texture>
    {
        switch (slot)
        {
        case TextureSlot::BaseColor: return baseColorTexture;
        case TextureSlot::Specular: return metallicRoughnessTexture;
        case TextureSlot::Emissive: return emissiveTexture;
        case TextureSlot::Normal: return normalTexture;
        case TextureSlot::Transmission: return transmissionTexture;
        case TextureSlot::Displacement: return displacementTexture;
        default: return nullptr;
        }
    }

    auto BasicMaterial::bindTextures(ShaderVariable& var) const -> void
    {
        if (baseColorTexture && var.hasMember("baseColorTexture"))
        {
            var["baseColorTexture"].setTexture(baseColorTexture);
        }

        if (metallicRoughnessTexture && var.hasMember("metallicRoughnessTexture"))
        {
            var["metallicRoughnessTexture"].setTexture(metallicRoughnessTexture);
        }

        if (normalTexture && var.hasMember("normalTexture"))
        {
            var["normalTexture"].setTexture(normalTexture);
        }

        if (emissiveTexture && var.hasMember("emissiveTexture"))
        {
            var["emissiveTexture"].setTexture(emissiveTexture);
        }

        if (transmissionTexture && var.hasMember("transmissionTexture"))
        {
            var["transmissionTexture"].setTexture(transmissionTexture);
        }

        if (displacementTexture && var.hasMember("displacementTexture"))
        {
            var["displacementTexture"].setTexture(displacementTexture);
        }
    }

    auto BasicMaterial::hasTextures() const -> bool
    {
        return baseColorTexture != nullptr
            || metallicRoughnessTexture != nullptr
            || normalTexture != nullptr
            || emissiveTexture != nullptr
            || transmissionTexture != nullptr
            || displacementTexture != nullptr;
    }

    auto BasicMaterial::getFlags() const -> uint32_t
    {
        uint32_t flags = static_cast<uint32_t>(generated::MaterialFlags::None);

        if (m_doubleSided)
        {
            flags |= static_cast<uint32_t>(generated::MaterialFlags::DoubleSided);
        }

        if (alphaMode == generated::AlphaMode::Mask)
        {
            flags |= static_cast<uint32_t>(generated::MaterialFlags::AlphaTested);
        }

        if (emissive.x > 0.0f || emissive.y > 0.0f || emissive.z > 0.0f)
        {
            flags |= static_cast<uint32_t>(generated::MaterialFlags::Emissive);
        }

        if (hasTextures())
        {
            flags |= static_cast<uint32_t>(generated::MaterialFlags::HasTextures);
        }

        return flags;
    }

    auto BasicMaterial::setDoubleSided(bool doubleSided) -> void
    {
        if (m_doubleSided != doubleSided)
        {
            m_doubleSided = doubleSided;
            markUpdates(MaterialUpdateFlags::DataChanged);
        }
    }

    auto BasicMaterial::isDoubleSided() const -> bool
    {
        return m_doubleSided;
    }

    auto BasicMaterial::setDescriptorHandles(
        uint32_t baseColorTextureHandle,
        uint32_t specularTextureHandle,
        uint32_t normalTextureHandle,
        uint32_t emissiveTextureHandle,
        uint32_t transmissionTextureHandle,
        uint32_t displacementTextureHandle,
        uint32_t samplerHandle
    ) -> void
    {
        m_baseColorTextureHandle = baseColorTextureHandle;
        m_specularTextureHandle = specularTextureHandle;
        m_normalTextureHandle = normalTextureHandle;
        m_emissiveTextureHandle = emissiveTextureHandle;
        m_transmissionTextureHandle = transmissionTextureHandle;
        m_displacementTextureHandle = displacementTextureHandle;
        m_samplerHandle = samplerHandle;
    }

    auto BasicMaterial::writeCommonData(generated::BasicMaterialData& data) const -> void
    {
        data.flags = 0;
        data.emissiveFactor = 1.0f;

        data.baseColor = baseColor;
        data.specular = float4{0.0f, 0.5f, 0.0f, 0.0f};
        data.specularTransmission = specularTransmission;
        data.emissive = emissive;
        data.diffuseTransmission = diffuseTransmission;
        data.transmission = transmission;

        data.volumeScattering = float3{0.0f, 0.0f, 0.0f};
        data.volumeAbsorption = float3{0.0f, 0.0f, 0.0f};
        data.volumeAnisotropy = 0.0f;

        data.displacementScale = 0.0f;
        data.displacementOffset = 0.0f;

        data.texBaseColor = makeTextureHandle(m_baseColorTextureHandle);
        data.texSpecular = makeTextureHandle(m_specularTextureHandle);
        data.texEmissive = makeTextureHandle(m_emissiveTextureHandle);
        data.texNormalMap = makeTextureHandle(m_normalTextureHandle);
        data.texTransmission = makeTextureHandle(m_transmissionTextureHandle);
        data.texDisplacementMap = makeTextureHandle(m_displacementTextureHandle);

        data.setNormalMapType(m_normalTextureHandle != 0 ? generated::NormalMapType::RG : generated::NormalMapType::None);
        data.setDisplacementMinSamplerID(m_samplerHandle);
        data.setDisplacementMaxSamplerID(m_samplerHandle);
    }

}
