#pragma once

#include "i-material.hpp"
#include "generated/material/basic-material-data.generated.hpp"
#include "rhi/texture.hpp"

#include <cstring>

namespace april::graphics
{
    class BasicMaterial : public IMaterial
    {
        APRIL_OBJECT(BasicMaterial)
    public:
        ~BasicMaterial() override = default;

        auto update(MaterialSystem* pOwner) -> MaterialUpdateFlags override;
        auto getTextureSlotInfo(TextureSlot slot) const -> TextureSlotInfo const& override;
        auto setTexture(TextureSlot slot, core::ref<Texture> texture) -> bool override;
        auto getTexture(TextureSlot slot) const -> core::ref<Texture> override;

        auto bindTextures(ShaderVariable& var) const -> void override;
        auto hasTextures() const -> bool override;
        auto getFlags() const -> uint32_t override;
        auto setDoubleSided(bool doubleSided) -> void override;
        auto isDoubleSided() const -> bool override;
        auto getMaxTextureCount() const -> size_t override { return 6; }

        auto setDescriptorHandles(
            uint32_t baseColorTextureHandle,
            uint32_t specularTextureHandle,
            uint32_t normalTextureHandle,
            uint32_t emissiveTextureHandle,
            uint32_t transmissionTextureHandle,
            uint32_t displacementTextureHandle,
            uint32_t samplerHandle
        ) -> void;

        // Shared parameter payload (Falcor BasicMaterial-style host base).
        float4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
        float ior{1.5f};
        float specularTransmission{0.0f};
        float3 emissive{0.0f, 0.0f, 0.0f};
        float diffuseTransmission{0.0f};
        float3 transmission{1.0f, 1.0f, 1.0f};
        float alphaCutoff{0.5f};
        generated::AlphaMode alphaMode{generated::AlphaMode::Opaque};

        core::ref<Texture> baseColorTexture{};
        core::ref<Texture> metallicRoughnessTexture{};
        core::ref<Texture> normalTexture{};
        core::ref<Texture> emissiveTexture{};
        core::ref<Texture> transmissionTexture{};
        core::ref<Texture> displacementTexture{};

    protected:
        template<typename T>
        auto prepareDataBlob(T const& payload) const -> generated::MaterialDataBlob
        {
            static_assert(sizeof(T) <= sizeof(generated::MaterialPayload));

            generated::MaterialDataBlob blob{};
            std::memcpy(&blob.payload, &payload, sizeof(T));
            return blob;
        }

        auto writeCommonData(generated::BasicMaterialData& data) const -> void;
        auto getSamplerHandle() const -> uint32_t { return m_samplerHandle; }
        auto getBaseColorTextureHandle() const -> uint32_t { return m_baseColorTextureHandle; }
        auto hasEmissive() const -> bool { return emissive.x > 0.0f || emissive.y > 0.0f || emissive.z > 0.0f; }
        auto hasActiveTextures() const -> bool { return hasTextures(); }

    private:
        bool m_doubleSided{false};
        uint32_t m_baseColorTextureHandle{0};
        uint32_t m_specularTextureHandle{0};
        uint32_t m_normalTextureHandle{0};
        uint32_t m_emissiveTextureHandle{0};
        uint32_t m_transmissionTextureHandle{0};
        uint32_t m_displacementTextureHandle{0};
        uint32_t m_samplerHandle{0};
    };
}
