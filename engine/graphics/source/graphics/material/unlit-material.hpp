#pragma once

#include "i-material.hpp"

namespace april::graphics
{
    class UnlitMaterial : public IMaterial
    {
        APRIL_OBJECT(UnlitMaterial)
    public:
        UnlitMaterial() = default;
        ~UnlitMaterial() override = default;

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

        float4 color{1.0f, 1.0f, 1.0f, 1.0f};
        float3 emissive{0.0f, 0.0f, 0.0f};

    private:
        bool m_doubleSided{false};
    };
}
