// Material factory helpers to create IMaterialInstance implementations.

import material.i_material;
import material.material_data;
import material.material_system;
import material.shading_data;
import material.standard_material;
import material.unlit_material;

struct MaterialFactory
{
    MaterialSystemContext ms;

    __init(MaterialSystemContext materialSystem)
    {
        ms = materialSystem;
    }

    StandardMaterialInstance getStandardMaterialInstance(uint materialID, ShadingData sd)
    {
        StandardMaterialData mtl = getMaterial(ms, materialID);
        SamplerState sampler = getMaterialSampler(ms, mtl);

        float4 baseColor = mtl.baseColor;
        if ((mtl.header.flags & (uint)MaterialFlags.HasTextures) != 0)
        {
            baseColor *= sampleMaterialTexture(ms, mtl.baseColorTextureHandle, sampler, sd.uv);
        }

        float metallic = mtl.metallic;
        float roughness = mtl.roughness;
        if ((mtl.header.flags & (uint)MaterialFlags.HasTextures) != 0)
        {
            float4 mrSample = sampleMaterialTexture(ms, mtl.metallicRoughnessTextureHandle, sampler, sd.uv);
            roughness *= mrSample.g;
            metallic *= mrSample.b;
        }

        float3 emissive = mtl.emissive;
        if ((mtl.header.flags & (uint)MaterialFlags.Emissive) != 0)
        {
            emissive *= sampleMaterialTexture(ms, mtl.emissiveTextureHandle, sampler, sd.uv).rgb;
        }

        return StandardMaterialInstance(mtl, baseColor.rgb, emissive, metallic, roughness);
    }
};

IMaterialInstance getMaterialInstance(MaterialFactory factory, uint materialID, ShadingData sd)
{
    StandardMaterialData mtl = getMaterial(factory.ms, materialID);
    if (MaterialType(mtl.header.materialType) == MaterialType::Unlit)
    {
        UnlitMaterialInstance unlit = UnlitMaterialInstance(mtl);
        return createDynamicObject<IMaterialInstance>(unlit);
    }

    StandardMaterialInstance standard = factory.getStandardMaterialInstance(materialID, sd);
    return createDynamicObject<IMaterialInstance>(standard);
}
