// Unlit material implementation.

import material.material_types;
import material.material_data;
import material.shading_data;
import material.i_material;

struct UnlitMaterialInstance : IMaterialInstance
{
    StandardMaterialData data;

    __init(StandardMaterialData data_)
    {
        data = data_;
    }

    MaterialType getType()
    {
        return MaterialType::Unlit;
    }

    float3 eval(ShadingData sd, float3 wo)
    {
        return float3(0.0f);
    }

    bool sample(ShadingData sd, float3 u, out float3 wo, out float pdf, out float3 weight, out uint lobeType)
    {
        wo = sd.N;
        pdf = 0.0f;
        weight = float3(0.0f);
        lobeType = (uint)LobeType::None;
        return false;
    }

    float evalPdf(ShadingData sd, float3 wo)
    {
        return 0.0f;
    }

    float3 getEmission(ShadingData sd)
    {
        return data.emissive;
    }

    float3 getAlbedo(ShadingData sd)
    {
        return data.baseColor.rgb;
    }

    float getRoughness(ShadingData sd)
    {
        return 1.0f;
    }
};
