#pragma once

#include "i-material.hpp"
#include "generated/material/basic-material-data.generated.hpp"

namespace april::graphics
{
    class Device;
    class SceneCache;

    class BasicMaterial : public Material
    {
        APRIL_OBJECT(BasicMaterial)
    public:
        ~BasicMaterial() override = default;

        auto update(MaterialSystem* p_owner) -> MaterialUpdateFlags override;
        auto isDisplaced() const -> bool override;
        auto isEqual(core::ref<Material> const& p_other) const -> bool override;
        auto setAlphaMode(generated::AlphaMode alphaMode) -> void override;
        auto setAlphaThreshold(float alphaThreshold) -> void override;
        auto setTexture(TextureSlot const slot, core::ref<Texture> const& p_texture) -> bool override;
        auto optimizeTexture(TextureSlot const slot, TextureAnalyzer::Result const& texInfo, TextureOptimizationStats& stats) -> void override;
        auto setDefaultTextureSampler(core::ref<Sampler> const& p_sampler) -> void override;
        auto getDefaultTextureSampler() const -> core::ref<Sampler> override { return m_pDefaultSampler; }

        auto getDataBlob() const -> generated::MaterialDataBlob override { return prepareDataBlob(m_data); }

        auto setBaseColorTexture(core::ref<Texture> const& pBaseColor) -> void { (void)setTexture(TextureSlot::BaseColor, pBaseColor); }
        auto getBaseColorTexture() const -> core::ref<Texture> { return getTexture(TextureSlot::BaseColor); }
        auto setSpecularTexture(core::ref<Texture> const& pSpecular) -> void { (void)setTexture(TextureSlot::Specular, pSpecular); }
        auto getSpecularTexture() const -> core::ref<Texture> { return getTexture(TextureSlot::Specular); }
        auto setEmissiveTexture(core::ref<Texture> const& pEmissive) -> void { (void)setTexture(TextureSlot::Emissive, pEmissive); }
        auto getEmissiveTexture() const -> core::ref<Texture> { return getTexture(TextureSlot::Emissive); }
        auto setTransmissionTexture(core::ref<Texture> const& pTransmission) -> void { (void)setTexture(TextureSlot::Transmission, pTransmission); }
        auto getTransmissionTexture() const -> core::ref<Texture> { return getTexture(TextureSlot::Transmission); }
        auto setNormalMap(core::ref<Texture> const& pNormalMap) -> void { (void)setTexture(TextureSlot::Normal, pNormalMap); }
        auto getNormalMap() const -> core::ref<Texture> { return getTexture(TextureSlot::Normal); }
        auto setDisplacementMap(core::ref<Texture> const& pDisplacementMap) -> void { (void)setTexture(TextureSlot::Displacement, pDisplacementMap); }
        auto getDisplacementMap() const -> core::ref<Texture> { return getTexture(TextureSlot::Displacement); }

        auto setDisplacementScale(float scale) -> void;
        auto getDisplacementScale() const -> float { return m_data.displacementScale; }
        auto setDisplacementOffset(float offset) -> void;
        auto getDisplacementOffset() const -> float { return m_data.displacementOffset; }

        auto setBaseColor(float4 const& color) -> void;
        auto setBaseColor3(float3 const& color) -> void { setBaseColor(float4(color, getBaseColor().w)); }
        auto getBaseColor() const -> float4 { return float4(m_data.baseColor); }
        auto getBaseColor3() const -> float3
        {
            auto const c = getBaseColor();
            return float3(c.x, c.y, c.z);
        }

        auto setSpecularParams(float4 const& value) -> void;
        auto getSpecularParams() const -> float4 { return float4(m_data.specular); }

        auto setTransmissionColor(float3 const& transmissionColor) -> void;
        auto getTransmissionColor() const -> float3 { return float3(m_data.transmission); }

        auto setDiffuseTransmission(float value) -> void;
        auto getDiffuseTransmission() const -> float { return float(m_data.diffuseTransmission); }

        auto setSpecularTransmission(float value) -> void;
        auto getSpecularTransmission() const -> float { return float(m_data.specularTransmission); }

        auto setVolumeAbsorption(float3 const& volumeAbsorption) -> void;
        auto getVolumeAbsorption() const -> float3 { return float3(m_data.volumeAbsorption); }

        auto setVolumeScattering(float3 const& volumeScattering) -> void;
        auto getVolumeScattering() const -> float3 { return float3(m_data.volumeScattering); }

        auto setVolumeAnisotropy(float volumeAnisotropy) -> void;
        auto getVolumeAnisotropy() const -> float { return float(m_data.volumeAnisotropy); }

        auto getNormalMapType() const -> generated::NormalMapType { return m_data.getNormalMapType(); }
        auto getData() const -> generated::BasicMaterialData const& { return m_data; }

        auto operator==(BasicMaterial const& other) const -> bool;

    protected:
        BasicMaterial(core::ref<Device> p_device, std::string const& name, generated::MaterialType type);

        auto isAlphaSupported() const -> bool;
        auto prepareDisplacementMapForRendering() -> void;
        auto adjustDoubleSidedFlag() -> void;
        auto updateAlphaMode() -> void;
        auto updateNormalMapType() -> void;
        auto updateEmissiveFlag() -> void;
        virtual auto updateDeltaSpecularFlag() -> void {}
        virtual auto setEmissiveColor(float3 const& color) -> void { (void)color; }

        generated::BasicMaterialData m_data{};

        core::ref<Sampler> m_pDefaultSampler{};
        core::ref<Sampler> m_pDisplacementMinSampler{};
        core::ref<Sampler> m_pDisplacementMaxSampler{};

        float2 m_alphaRange{0.f, 1.f};
        bool m_isTexturedBaseColorConstant{false};
        bool m_isTexturedAlphaConstant{false};
        bool m_displacementMapChanged{false};

        friend class SceneCache;
    };
}
