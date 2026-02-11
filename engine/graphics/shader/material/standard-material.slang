// StandardMaterial - Standard PBR material implementation.

import material.material_types;
import material.material_data;
import material.basic_material_data;
import material.shading_data;
import material.material_system;
import material.i_material;
import material.i_material_instance;
import material.texture_sampler;
import material.material_instance_hints;
import material.standard_material_instance;

struct StandardMaterial : IMaterial
{
    MaterialDataBlob materialBlob;
    typedef StandardMaterialInstance MaterialInstance;

    __init(MaterialDataBlob materialBlob_)
    {
        materialBlob = materialBlob_;
    }

    MaterialType getType()
    {
        return materialBlob.header.getMaterialType();
    }

    StandardMaterialInstance setupMaterialInstance(
        const MaterialSystem ms,
        const ShadingData sd,
        const ITextureSampler lod,
        uint hints = (uint)MaterialInstanceHints::None
    )
    {
        BasicMaterialData data = reinterpret<BasicMaterialData, MaterialPayload>(materialBlob.payload);
        SamplerState sampler = ms.getTextureSampler(materialBlob.header.getDefaultTextureSamplerID());

        float4 baseColor = ms.sampleTexture(data.texBaseColor, sampler, sd.uv, float4(data.baseColor), lod);
        float4 specular = ms.sampleTexture(data.texSpecular, sampler, sd.uv, float4(data.specular), lod);
        float3 emissive = ms.sampleTexture(data.texEmissive, sampler, sd.uv, float4(data.emissive, 1.0f), lod).rgb * data.emissiveFactor;

        float metallic = saturate(specular.b);
        float roughness = saturate(specular.g);

        return StandardMaterialInstance(materialBlob.header, data, baseColor.rgb, emissive, metallic, roughness);
    }

    StandardMaterialInstance setupDiffMaterialInstance(
        out DiffMaterialData diffData,
        const MaterialSystem ms,
        const ShadingData sd,
        const ITextureSampler lod,
        uint hints = (uint)MaterialInstanceHints::None
    )
    {
        diffData.payload = 0.0f;
        return setupMaterialInstance(ms, sd, lod, hints);
    }

    VolumeProperties getHomogeneousVolumeProperties(const MaterialSystem ms, const uint materialID)
    {
        BasicMaterialData data = reinterpret<BasicMaterialData, MaterialPayload>(materialBlob.payload);
        VolumeProperties p;
        p.sigmaA = data.volumeAbsorption;
        p.sigmaS = data.volumeScattering;
        p.g = data.volumeAnisotropy;
        return p;
    }
};
