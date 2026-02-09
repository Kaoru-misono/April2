// StandardMaterial - Standard PBR material implementation.

#pragma once

#include "i-material.hpp"
#include "rhi/texture.hpp"

#include <asset/material-asset.hpp>

namespace april::graphics
{
    class Device;

    /**
     * Standard PBR material using metallic-roughness workflow.
     * This is the default material type for most use cases.
     */
    class StandardMaterial : public IMaterial
    {
        APRIL_OBJECT(StandardMaterial)
    public:
        /**
         * Diffuse BRDF model selection.
         * Allows choosing different diffuse implementations via type conformances.
         */
        enum class DiffuseBRDFModel
        {
            Lambert,    // Simple Lambertian diffuse
            Frostbite,  // Energy-conserving Frostbite diffuse
        };

        StandardMaterial();
        ~StandardMaterial() override = default;

        /**
         * Create a standard material from an asset.
         * @param device Graphics device for texture creation.
         * @param asset Material asset containing parameters and texture references.
         * @return Newly created material.
         */
        static auto createFromAsset(
            core::ref<Device> device,
            asset::MaterialAsset const& asset
        ) -> core::ref<StandardMaterial>;

        // IMaterial interface
        auto getType() const -> generated::MaterialType override;
        auto getTypeName() const -> std::string override;
        auto writeData(generated::StandardMaterialData& data) const -> void override;
        auto getTypeConformances() const -> TypeConformanceList override;
        auto bindTextures(ShaderVariable& var) const -> void override;
        auto hasTextures() const -> bool override;
        auto getFlags() const -> uint32_t override;
        auto setDoubleSided(bool doubleSided) -> void override;
        auto isDoubleSided() const -> bool override;
        auto serializeParameters(nlohmann::json& outJson) const -> void override;
        auto deserializeParameters(nlohmann::json const& inJson) -> bool override;

        auto setDescriptorHandles(
            uint32_t baseColorTextureHandle,
            uint32_t metallicRoughnessTextureHandle,
            uint32_t normalTextureHandle,
            uint32_t occlusionTextureHandle,
            uint32_t emissiveTextureHandle,
            uint32_t samplerHandle,
            uint32_t bufferHandle
        ) -> void;

        // PBR Parameters (matches generated::StandardMaterialData)
        float4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
        float metallic{0.0f};
        float roughness{0.5f};
        float ior{1.5f};
        float specularTransmission{0.0f};
        float3 emissive{0.0f, 0.0f, 0.0f};
        float diffuseTransmission{0.0f};
        float3 transmission{1.0f, 1.0f, 1.0f};
        float alphaCutoff{0.5f};
        float normalScale{1.0f};
        float occlusionStrength{1.0f};

        // Alpha mode
        generated::AlphaMode alphaMode{generated::AlphaMode::Opaque};

        // Textures
        core::ref<Texture> baseColorTexture{};
        core::ref<Texture> metallicRoughnessTexture{};
        core::ref<Texture> normalTexture{};
        core::ref<Texture> occlusionTexture{};
        core::ref<Texture> emissiveTexture{};

        // BSDF model selection
        DiffuseBRDFModel diffuseModel{DiffuseBRDFModel::Frostbite};

    private:
        bool m_doubleSided{false};
        uint32_t m_activeLobes{0};
        uint32_t m_baseColorTextureHandle{0};
        uint32_t m_metallicRoughnessTextureHandle{0};
        uint32_t m_normalTextureHandle{0};
        uint32_t m_occlusionTextureHandle{0};
        uint32_t m_emissiveTextureHandle{0};
        uint32_t m_samplerHandle{0};
        uint32_t m_bufferHandle{0};

        auto updateFlags() const -> uint32_t;
        auto updateActiveLobes() const -> uint32_t;
    };

} // namespace april::graphics
