#include "i-material.hpp"

#include "material-system.hpp"
#include "../rhi/texture.hpp"

#include <core/log/logger.hpp>

namespace april::graphics
{
    inline namespace material_checks
    {
        static_assert(sizeof(generated::TextureHandle) == 4);
        static_assert(sizeof(generated::MaterialHeader) == 16);
        static_assert(sizeof(generated::MaterialPayload) == 112);
        static_assert(sizeof(generated::MaterialDataBlob) == 128);
        static_assert(generated::MaterialHeader::kAlphaThresholdBits == 16);
    }

    auto operator==(generated::MaterialHeader const& lhs, generated::MaterialHeader const& rhs) -> bool
    {
        return lhs.packedData == rhs.packedData;
    }

    Material::Material(core::ref<Device> pDevice, std::string const& name, generated::MaterialType type)
        : m_device{std::move(pDevice)}
        , m_name{name}
    {
        m_header.packedData = uint4{0, 0, 0, 0};
        m_header.setMaterialType(type);
        m_header.setAlphaMode(generated::AlphaMode::Opaque);
        m_header.setAlphaThreshold(0.5f);
        m_header.setActiveLobes(static_cast<uint32_t>(generated::LobeType::All));
        m_header.setIoR(1.0f);
    }

    auto Material::setName(std::string const& name) -> void
    {
        m_name = name;
    }

    auto Material::getName() const -> std::string const&
    {
        return m_name;
    }

    auto Material::getType() const -> generated::MaterialType
    {
        return m_header.getMaterialType();
    }

    auto Material::isOpaque() const -> bool
    {
        return getAlphaMode() == generated::AlphaMode::Opaque;
    }

    auto Material::isDisplaced() const -> bool
    {
        return false;
    }

    auto Material::isEmissive() const -> bool
    {
        return m_header.isEmissive();
    }

    auto Material::isDynamic() const -> bool
    {
        return false;
    }

    auto Material::setDoubleSided(bool doubleSided) -> void
    {
        if (m_header.isDoubleSided() != doubleSided)
        {
            m_header.setDoubleSided(doubleSided);
            markUpdates(UpdateFlags::DataChanged);
        }
    }

    auto Material::isDoubleSided() const -> bool
    {
        return m_header.isDoubleSided();
    }

    auto Material::setThinSurface(bool thinSurface) -> void
    {
        if (m_header.isThinSurface() != thinSurface)
        {
            m_header.setThinSurface(thinSurface);
            markUpdates(UpdateFlags::DataChanged);
        }
    }

    auto Material::isThinSurface() const -> bool
    {
        return m_header.isThinSurface();
    }

    auto Material::setAlphaMode(generated::AlphaMode alphaMode) -> void
    {
        (void)alphaMode;
        AP_WARN("Material '{}' of type '{}' does not support alpha mode override. Ignoring setAlphaMode().", m_name, static_cast<uint32_t>(getType()));
    }

    auto Material::getAlphaMode() const -> generated::AlphaMode
    {
        return m_header.getAlphaMode();
    }

    auto Material::setAlphaThreshold(float alphaThreshold) -> void
    {
        (void)alphaThreshold;
        AP_WARN("Material '{}' of type '{}' does not support alpha threshold override. Ignoring setAlphaThreshold().", m_name, static_cast<uint32_t>(getType()));
    }

    auto Material::getAlphaThreshold() const -> float
    {
        return m_header.getAlphaThreshold();
    }

    auto Material::getAlphaTextureHandle() const -> generated::TextureHandle
    {
        return m_header.getAlphaTextureHandle();
    }

    auto Material::setNestedPriority(uint32_t priority) -> void
    {
        auto constexpr kMaxPriority = (1u << generated::MaterialHeader::kNestedPriorityBits) - 1u;
        if (priority > kMaxPriority)
        {
            AP_WARN("Requested nested priority {} for material '{}' is out of range. Clamping to {}.", priority, m_name, kMaxPriority);
            priority = kMaxPriority;
        }

        if (m_header.getNestedPriority() != priority)
        {
            m_header.setNestedPriority(priority);
            markUpdates(UpdateFlags::DataChanged);
        }
    }

    auto Material::getNestedPriority() const -> uint32_t
    {
        return m_header.getNestedPriority();
    }

    auto Material::setIndexOfRefraction(float ior) -> void
    {
        if (m_header.getIoR() != ior)
        {
            m_header.setIoR(ior);
            markUpdates(UpdateFlags::DataChanged);
        }
    }

    auto Material::getIndexOfRefraction() const -> float
    {
        return m_header.getIoR();
    }

    auto Material::getTextureSlotInfo(TextureSlot slot) const -> TextureSlotInfo const&
    {
        auto const i = static_cast<size_t>(slot);
        if (i >= m_textureSlotInfo.size())
        {
            static TextureSlotInfo const kDisabled{};
            return kDisabled;
        }

        return m_textureSlotInfo[i];
    }

    auto Material::hasTextureSlot(TextureSlot slot) const -> bool
    {
        return getTextureSlotInfo(slot).isEnabled();
    }

    auto Material::setTexture(TextureSlot slot, core::ref<Texture> const& pTexture) -> bool
    {
        if (!hasTextureSlot(slot))
        {
            AP_WARN("Material '{}' does not have texture slot '{}'. Ignoring setTexture().", m_name, static_cast<uint32_t>(slot));
            return false;
        }

        auto const i = static_cast<size_t>(slot);
        if (m_textureSlotData[i].pTexture == pTexture)
        {
            return false;
        }

        m_textureSlotData[i].pTexture = pTexture;
        markUpdates(UpdateFlags::ResourcesChanged);
        if (slot == TextureSlot::Emissive)
        {
            markUpdates(UpdateFlags::EmissiveChanged);
        }
        return true;
    }

    auto Material::loadTexture(TextureSlot slot, std::filesystem::path const& path, bool useSrgb) -> bool
    {
        (void)slot;
        (void)path;
        (void)useSrgb;
        return false;
    }

    auto Material::clearTexture(TextureSlot slot) -> void
    {
        (void)setTexture(slot, nullptr);
    }

    auto Material::getTexture(TextureSlot slot) const -> core::ref<Texture>
    {
        if (!hasTextureSlot(slot))
        {
            return nullptr;
        }

        return m_textureSlotData[static_cast<size_t>(slot)].pTexture;
    }

    auto Material::optimizeTexture(TextureSlot slot, TextureOptimizationStats& stats) -> void
    {
        (void)slot;
        (void)stats;
    }

    auto Material::setDefaultTextureSampler(core::ref<Sampler> const& pSampler) -> void
    {
        (void)pSampler;
    }

    auto Material::getDefaultTextureSampler() const -> core::ref<Sampler>
    {
        return nullptr;
    }

    auto Material::getHeader() const -> generated::MaterialHeader const&
    {
        return m_header;
    }

    auto Material::getDefines() const -> DefineList
    {
        return {};
    }

    auto Material::getMaxBufferCount() const -> size_t
    {
        return 0;
    }

    auto Material::getMaxTextureCount() const -> size_t
    {
        return static_cast<size_t>(TextureSlot::Count);
    }

    auto Material::getMaxTexture3DCount() const -> size_t
    {
        return 0;
    }

    auto Material::getMaterialInstanceByteSize() const -> size_t
    {
        return 128;
    }

    auto Material::getParamLayout() const -> MaterialParamLayout const&
    {
        static MaterialParamLayout const kEmpty{};
        return kEmpty;
    }

    auto Material::serializeParams() const -> SerializedMaterialParams
    {
        return {};
    }

    auto Material::deserializeParams(SerializedMaterialParams const& params) -> void
    {
        (void)params;
    }

    auto Material::registerUpdateCallback(UpdateCallback const& updateCallback) -> void
    {
        m_updateCallback = updateCallback;
    }

    auto Material::markUpdates(UpdateFlags updates) -> void
    {
        if (updates == UpdateFlags::None)
        {
            return;
        }

        m_updates |= updates;
        if (m_updateCallback)
        {
            m_updateCallback(updates);
        }
    }

    auto Material::hasTextureSlotData(TextureSlot slot) const -> bool
    {
        auto const i = static_cast<size_t>(slot);
        return i < m_textureSlotData.size() && m_textureSlotData[i].hasData();
    }

    auto Material::updateTextureHandle(MaterialSystem* pOwner, core::ref<Texture> const& pTexture, generated::TextureHandle& handle) -> void
    {
        auto const previous = handle;

        if (pOwner && pTexture)
        {
            auto const descriptor = pOwner->registerTextureDescriptor(pTexture);
            if (descriptor != MaterialSystem::kInvalidDescriptorHandle)
            {
                handle.setTextureID(descriptor);
                handle.setUdimEnabled(false);
            }
            else
            {
                handle.setMode(generated::TextureHandle::Mode::Uniform);
                handle.setUdimEnabled(false);
            }
        }
        else
        {
            handle.setMode(generated::TextureHandle::Mode::Uniform);
            handle.setUdimEnabled(false);
        }

        if (handle.packedData != previous.packedData)
        {
            m_updates |= UpdateFlags::DataChanged;
        }
    }

    auto Material::updateTextureHandle(MaterialSystem* pOwner, TextureSlot slot, generated::TextureHandle& handle) -> void
    {
        updateTextureHandle(pOwner, getTexture(slot), handle);

        if (slot == TextureSlot::BaseColor)
        {
            m_header.setAlphaTextureHandle(handle);
            m_updates |= UpdateFlags::DataChanged;
        }
    }

    auto Material::updateDefaultTextureSamplerID(MaterialSystem* pOwner, core::ref<Sampler> const& pSampler) -> void
    {
        if (!pOwner || !pSampler)
        {
            return;
        }

        auto const samplerID = pOwner->registerSamplerDescriptor(pSampler);
        if (m_header.getDefaultTextureSamplerID() != samplerID)
        {
            m_header.setDefaultTextureSamplerID(samplerID);
            m_updates |= UpdateFlags::DataChanged;
        }
    }

    auto Material::isBaseEqual(Material const& other) const -> bool
    {
        return m_header.packedData == other.m_header.packedData
            && m_textureSlotInfo == other.m_textureSlotInfo
            && m_textureSlotData == other.m_textureSlotData;
    }

    auto Material::detectNormalMapType(core::ref<Texture> const& pNormalMap) -> generated::NormalMapType
    {
        if (!pNormalMap)
        {
            return generated::NormalMapType::None;
        }

        auto const channelCount = getFormatChannelCount(pNormalMap->getFormat());
        if (channelCount == 2)
        {
            return generated::NormalMapType::RG;
        }

        if (channelCount == 3 || channelCount == 4)
        {
            return generated::NormalMapType::RGB;
        }

        AP_WARN("Unsupported normal map format '{}'.", to_string(pNormalMap->getFormat()));
        return generated::NormalMapType::None;
    }
}
