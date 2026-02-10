// StandardMaterial - Standard PBR material implementation.

import material.material_types;
import material.material_data;
import material.shading_data;
import material.material_system;
import material.i_material;
import material.i_material_instance;
import material.texture_sampler;
import material.material_instance_hints;
import material.standard_material_instance;

struct StandardMaterial : IMaterial
{
    uint materialID;
    typedef StandardMaterialInstance MaterialInstance;

    __init(uint materialID_)
    {
        materialID = materialID_;
    }

    MaterialType getType()
    {
        return MaterialType::Standard;
    }

    StandardMaterialInstance setupMaterialInstance(
        const MaterialSystemContext ms,
        const ShadingData sd,
        const ITextureSampler lod,
        uint hints = (uint)MaterialInstanceHints::None
    )
    {
        StandardMaterialData data = ms.getMaterialData(materialID);
        SamplerState sampler = ms.getMaterialSampler(data);

        float4 baseColor = data.baseColor;
        if ((data.header.flags & (uint)MaterialFlags.HasTextures) != 0)
        {
            baseColor *= ms.sampleMaterialTexture(data.baseColorTextureHandle, sampler, sd.uv, lod);
        }

        float metallic = data.metallic;
        float roughness = data.roughness;
        if ((data.header.flags & (uint)MaterialFlags.HasTextures) != 0)
        {
            float4 mrSample = ms.sampleMaterialTexture(data.metallicRoughnessTextureHandle, sampler, sd.uv, lod);
            roughness *= mrSample.g;
            metallic *= mrSample.b;
        }

        float3 emissive = data.emissive;
        if ((data.header.flags & (uint)MaterialFlags.Emissive) != 0)
        {
            emissive *= ms.sampleMaterialTexture(data.emissiveTextureHandle, sampler, sd.uv, lod).rgb;
        }

        return StandardMaterialInstance(data, baseColor.rgb, emissive, metallic, roughness);
    }

    StandardMaterialInstance setupDiffMaterialInstance(
        out DiffMaterialData diffData,
        const MaterialSystemContext ms,
        const ShadingData sd,
        const ITextureSampler lod,
        uint hints = (uint)MaterialInstanceHints::None
    )
    {
        diffData.payload = 0.0f;
        return setupMaterialInstance(ms, sd, lod, hints);
    }

    VolumeProperties getHomogeneousVolumeProperties(const MaterialSystemContext ms, const uint materialID)
    {
        VolumeProperties p;
        p.sigmaA = 0.0f;
        p.sigmaS = 0.0f;
        p.g = 0.0f;
        return p;
    }
};
