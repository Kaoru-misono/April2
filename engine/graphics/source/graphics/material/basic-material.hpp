#pragma once

#include "i-material.hpp"
#include "rhi/texture.hpp"

namespace april::graphics
{
    class BasicMaterial : public IMaterial
    {
        APRIL_OBJECT(BasicMaterial)
    public:
        ~BasicMaterial() override = default;

        auto bindTextures(ShaderVariable& var) const -> void override;
        auto hasTextures() const -> bool override;
        auto getFlags() const -> uint32_t override;
        auto setDoubleSided(bool doubleSided) -> void override;
        auto isDoubleSided() const -> bool override;

        auto setDescriptorHandles(
            uint32_t baseColorTextureHandle,
            uint32_t metallicRoughnessTextureHandle,
            uint32_t normalTextureHandle,
            uint32_t occlusionTextureHandle,
            uint32_t emissiveTextureHandle,
            uint32_t samplerHandle,
            uint32_t bufferHandle
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
        core::ref<Texture> occlusionTexture{};
        core::ref<Texture> emissiveTexture{};

    protected:
        auto writeCommonData(generated::StandardMaterialData& data) const -> void;
        auto serializeCommonParameters(nlohmann::json& outJson) const -> void;
        auto deserializeCommonParameters(nlohmann::json const& inJson) -> void;

    private:
        bool m_doubleSided{false};
        uint32_t m_baseColorTextureHandle{0};
        uint32_t m_metallicRoughnessTextureHandle{0};
        uint32_t m_normalTextureHandle{0};
        uint32_t m_occlusionTextureHandle{0};
        uint32_t m_emissiveTextureHandle{0};
        uint32_t m_samplerHandle{0};
        uint32_t m_bufferHandle{0};
    };
}
