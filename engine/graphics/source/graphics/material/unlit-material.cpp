#include "unlit-material.hpp"

namespace april::graphics
{
    auto UnlitMaterial::getType() const -> generated::MaterialType
    {
        return generated::MaterialType::Unlit;
    }

    auto UnlitMaterial::getTypeName() const -> std::string
    {
        return "Unlit";
    }

    auto UnlitMaterial::writeData(generated::StandardMaterialData& data) const -> void
    {
        data = {};
        data.header.abiVersion = generated::kMaterialAbiVersion;
        data.header.materialType = static_cast<uint32_t>(generated::MaterialType::Unlit);
        data.header.flags = getFlags();
        data.header.alphaMode = static_cast<uint32_t>(generated::AlphaMode::Opaque);
        data.baseColor = color;
        data.emissive = emissive;
    }

    auto UnlitMaterial::getTypeConformances() const -> TypeConformanceList
    {
        TypeConformanceList conformances;
        conformances.add(
            "UnlitMaterial",
            "IMaterial",
            static_cast<uint32_t>(generated::MaterialType::Unlit)
        );
        conformances.add("UnlitMaterialInstance", "IMaterialInstance");
        return conformances;
    }

    auto UnlitMaterial::getShaderModules() const -> ProgramDesc::ShaderModuleList
    {
        ProgramDesc::ShaderModuleList modules;
        modules.push_back(ProgramDesc::ShaderModule::fromFile("engine/graphics/shader/material/i-material-instance.slang"));
        modules.push_back(ProgramDesc::ShaderModule::fromFile("engine/graphics/shader/material/unlit-material-instance.slang"));
        modules.push_back(ProgramDesc::ShaderModule::fromFile("engine/graphics/shader/material/unlit-material.slang"));
        return modules;
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
        if (m_doubleSided != doubleSided)
        {
            m_doubleSided = doubleSided;
            markDirty(MaterialUpdateFlags::DataChanged);
        }
    }

    auto UnlitMaterial::isDoubleSided() const -> bool
    {
        return m_doubleSided;
    }

    auto UnlitMaterial::serializeParameters(nlohmann::json& outJson) const -> void
    {
        outJson["type"] = "Unlit";
        outJson["color"] = color;
        outJson["emissive"] = emissive;
        outJson["doubleSided"] = m_doubleSided;
    }

    auto UnlitMaterial::deserializeParameters(nlohmann::json const& inJson) -> bool
    {
        if (inJson.contains("color"))
        {
            color = inJson["color"].get<float4>();
        }
        if (inJson.contains("emissive"))
        {
            emissive = inJson["emissive"].get<float3>();
        }
        if (inJson.contains("doubleSided"))
        {
            m_doubleSided = inJson["doubleSided"].get<bool>();
        }
        markDirty(MaterialUpdateFlags::All);
        return true;
    }
}
