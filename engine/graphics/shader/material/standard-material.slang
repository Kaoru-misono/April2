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
    StandardMaterialData data;

    __init(StandardMaterialData data_)
    {
        data = data_;
    }

    MaterialType getType()
    {
        return MaterialType::Standard;
    }

    IMaterialInstance setupMaterialInstance(
        MaterialSystemContext ms,
        ShadingData sd,
        ITextureSampler lod,
        uint hints = (uint)MaterialInstanceHints::None
    )
    {
        SamplerState sampler = getMaterialSampler(ms, data);

        float4 baseColor = data.baseColor;
        if ((data.header.flags & (uint)MaterialFlags.HasTextures) != 0)
        {
            baseColor *= sampleMaterialTexture(ms, data.baseColorTextureHandle, sampler, sd.uv, lod);
        }

        float metallic = data.metallic;
        float roughness = data.roughness;
        if ((data.header.flags & (uint)MaterialFlags.HasTextures) != 0)
        {
            float4 mrSample = sampleMaterialTexture(ms, data.metallicRoughnessTextureHandle, sampler, sd.uv, lod);
            roughness *= mrSample.g;
            metallic *= mrSample.b;
        }

        float3 emissive = data.emissive;
        if ((data.header.flags & (uint)MaterialFlags.Emissive) != 0)
        {
            emissive *= sampleMaterialTexture(ms, data.emissiveTextureHandle, sampler, sd.uv, lod).rgb;
        }

        return StandardMaterialInstance(data, baseColor.rgb, emissive, metallic, roughness);
    }

    IMaterialInstance setupDiffMaterialInstance(
        out DiffMaterialData diffData,
        MaterialSystemContext ms,
        ShadingData sd,
        ITextureSampler lod,
        uint hints = (uint)MaterialInstanceHints::None
    )
    {
        diffData.payload = 0.0f;
        return setupMaterialInstance(ms, sd, lod, hints);
    }

    VolumeProperties getHomogeneousVolumeProperties(MaterialSystemContext ms, uint materialID)
    {
        VolumeProperties p;
        p.sigmaA = 0.0f;
        p.sigmaS = 0.0f;
        p.g = 0.0f;
        return p;
    }
};
