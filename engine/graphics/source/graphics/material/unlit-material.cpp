#include "unlit-material.hpp"

namespace april::graphics
{
    auto UnlitMaterial::getType() const -> generated::MaterialType
    {
        return generated::MaterialType::Unlit;
    }

    auto UnlitMaterial::writeData(generated::StandardMaterialData& data) const -> void
    {
        data = {};
        data.header.abiVersion = 1;
        data.header.materialType = static_cast<uint32_t>(generated::MaterialType::Unlit);
        data.header.flags = getFlags();
        data.header.alphaMode = static_cast<uint32_t>(generated::AlphaMode::Opaque);
        data.baseColor = color;
        data.emissive = emissive;
    }

    auto UnlitMaterial::getTypeConformances() const -> TypeConformanceList
    {
        TypeConformanceList conformances;
        conformances.add("UnlitMaterialInstance", "IMaterialInstance");
        return conformances;
    }

    auto UnlitMaterial::bindTextures(ShaderVariable& var) const -> void
    {
        (void)var;
    }

    auto UnlitMaterial::hasTextures() const -> bool
    {
        return false;
    }

    auto UnlitMaterial::getFlags() const -> uint32_t
    {
        auto flags = static_cast<uint32_t>(generated::MaterialFlags::None);
        if (m_doubleSided)
        {
            flags |= static_cast<uint32_t>(generated::MaterialFlags::DoubleSided);
        }
        if (emissive.x > 0.0f || emissive.y > 0.0f || emissive.z > 0.0f)
        {
            flags |= static_cast<uint32_t>(generated::MaterialFlags::Emissive);
        }
        return flags;
    }

    auto UnlitMaterial::setDoubleSided(bool doubleSided) -> void
    {
        m_doubleSided = doubleSided;
    }

    auto UnlitMaterial::isDoubleSided() const -> bool
    {
        return m_doubleSided;
    }
}
