// StandardMaterial - Standard PBR material implementation.

#pragma once

#include "basic-material.hpp"

#include <asset/material-asset.hpp>

namespace april::graphics
{
    class Device;

    /**
     * Standard PBR material using metallic-roughness workflow.
     * This is the default material type for most use cases.
     */
    class StandardMaterial : public BasicMaterial
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
        auto getShaderModules() const -> ProgramDesc::ShaderModuleList override;
        auto serializeParameters(nlohmann::json& outJson) const -> void override;
        auto deserializeParameters(nlohmann::json const& inJson) -> bool override;

        // PBR Parameters (matches generated::StandardMaterialData)
        float metallic{0.0f};
        float roughness{0.5f};
        float normalScale{1.0f};
        float occlusionStrength{1.0f};

        // BSDF model selection
        DiffuseBRDFModel diffuseModel{DiffuseBRDFModel::Frostbite};

    private:
        uint32_t m_activeLobes{0};
        auto updateActiveLobes() const -> uint32_t;
    };

} // namespace april::graphics
