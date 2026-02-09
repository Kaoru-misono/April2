#include "basic-material.hpp"

#include "program/shader-variable.hpp"

namespace april::graphics
{
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

        if (occlusionTexture && var.hasMember("occlusionTexture"))
        {
            var["occlusionTexture"].setTexture(occlusionTexture);
        }

        if (emissiveTexture && var.hasMember("emissiveTexture"))
        {
            var["emissiveTexture"].setTexture(emissiveTexture);
        }
    }

    auto BasicMaterial::hasTextures() const -> bool
    {
        return baseColorTexture != nullptr
            || metallicRoughnessTexture != nullptr
            || normalTexture != nullptr
            || occlusionTexture != nullptr
            || emissiveTexture != nullptr;
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
            markDirty(MaterialUpdateFlags::DataChanged);
        }
    }

    auto BasicMaterial::isDoubleSided() const -> bool
    {
        return m_doubleSided;
    }

    auto BasicMaterial::setDescriptorHandles(
        uint32_t baseColorTextureHandle,
        uint32_t metallicRoughnessTextureHandle,
        uint32_t normalTextureHandle,
        uint32_t occlusionTextureHandle,
        uint32_t emissiveTextureHandle,
        uint32_t samplerHandle,
        uint32_t bufferHandle
    ) -> void
    {
        m_baseColorTextureHandle = baseColorTextureHandle;
        m_metallicRoughnessTextureHandle = metallicRoughnessTextureHandle;
        m_normalTextureHandle = normalTextureHandle;
        m_occlusionTextureHandle = occlusionTextureHandle;
        m_emissiveTextureHandle = emissiveTextureHandle;
        m_samplerHandle = samplerHandle;
        m_bufferHandle = bufferHandle;
    }

    auto BasicMaterial::writeCommonData(generated::StandardMaterialData& data) const -> void
    {
        data.header.abiVersion = generated::kMaterialAbiVersion;
        data.header.flags = getFlags();
        data.header.alphaMode = static_cast<uint32_t>(alphaMode);
        data.header.reserved0 = 0;
        data.header.reserved1 = 0;
        data.header.reserved2 = 0;

        data.baseColor = baseColor;
        data.ior = ior;
        data.specularTransmission = specularTransmission;
        data.emissive = emissive;
        data.diffuseTransmission = diffuseTransmission;
        data.transmission = transmission;
        data.alphaCutoff = alphaCutoff;

        data.baseColorTextureHandle = m_baseColorTextureHandle;
        data.metallicRoughnessTextureHandle = m_metallicRoughnessTextureHandle;
        data.normalTextureHandle = m_normalTextureHandle;
        data.occlusionTextureHandle = m_occlusionTextureHandle;
        data.emissiveTextureHandle = m_emissiveTextureHandle;
        data.samplerHandle = m_samplerHandle;
        data.bufferHandle = m_bufferHandle;
        data.reservedDescriptorHandle = 0;
    }

    auto BasicMaterial::serializeCommonParameters(nlohmann::json& outJson) const -> void
    {
        outJson["baseColor"] = baseColor;
        outJson["ior"] = ior;
        outJson["specularTransmission"] = specularTransmission;
        outJson["emissive"] = emissive;
        outJson["diffuseTransmission"] = diffuseTransmission;
        outJson["transmission"] = transmission;
        outJson["alphaCutoff"] = alphaCutoff;
        outJson["doubleSided"] = m_doubleSided;

        switch (alphaMode)
        {
        case generated::AlphaMode::Opaque: outJson["alphaMode"] = "OPAQUE"; break;
        case generated::AlphaMode::Mask: outJson["alphaMode"] = "MASK"; break;
        case generated::AlphaMode::Blend: outJson["alphaMode"] = "BLEND"; break;
        }
    }

    auto BasicMaterial::deserializeCommonParameters(nlohmann::json const& inJson) -> void
    {
        if (inJson.contains("baseColor")) baseColor = inJson["baseColor"].get<float4>();
        if (inJson.contains("ior")) ior = inJson["ior"].get<float>();
        if (inJson.contains("specularTransmission")) specularTransmission = inJson["specularTransmission"].get<float>();
        if (inJson.contains("emissive")) emissive = inJson["emissive"].get<float3>();
        if (inJson.contains("diffuseTransmission")) diffuseTransmission = inJson["diffuseTransmission"].get<float>();
        if (inJson.contains("transmission")) transmission = inJson["transmission"].get<float3>();
        if (inJson.contains("alphaCutoff")) alphaCutoff = inJson["alphaCutoff"].get<float>();
        if (inJson.contains("doubleSided")) m_doubleSided = inJson["doubleSided"].get<bool>();

        if (inJson.contains("alphaMode"))
        {
            auto const mode = inJson["alphaMode"].get<std::string>();
            if (mode == "OPAQUE") alphaMode = generated::AlphaMode::Opaque;
            else if (mode == "MASK") alphaMode = generated::AlphaMode::Mask;
            else if (mode == "BLEND") alphaMode = generated::AlphaMode::Blend;
        }
    }
}
