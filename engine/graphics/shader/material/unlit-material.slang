// Unlit material implementation.

import material.material_types;
import material.material_data;
import material.shading_data;
import material.material_system;
import material.i_material;
import material.bsdf.sample_generator;

struct UnlitMaterialInstance;

struct UnlitMaterial : IMaterial
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

    IMaterialInstance setupMaterialInstance(MaterialSystemContext ms, ShadingData sd)
    {
        return UnlitMaterialInstance(data);
    }
};

struct NullBSDF : IBSDF
{
    float3 eval<S : ISampleGenerator>(float3 wi, float3 wo, inout S sg, BSDFContext bc)
    {
        return float3(0.0f);
    }

    bool sample<S : ISampleGenerator>(
        float3 wi,
        out float3 wo,
        out float pdf,
        out float3 weight,
        out uint lobeType,
        inout S sg,
        BSDFContext bc
    )
    {
        wo = float3(0.0f);
        pdf = 0.0f;
        weight = float3(0.0f);
        lobeType = (uint)LobeType::None;
        return false;
    }

    float evalPdf(float3 wi, float3 wo, BSDFContext bc)
    {
        return 0.0f;
    }

    AlbedoContributions evalAlbedo(float3 wi, LobeType lobeType)
    {
        return AlbedoContributions(float3(0.0f), float3(0.0f), float3(0.0f), float3(1.0f));
    }

    float3 getIorAsReflectance()
    {
        return float3(0.0f);
    }

    RoughnessInformation getRoughnessInformation(float3 wi)
    {
        return RoughnessInformation();
    }
};

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

    IBSDF getBSDF(ShadingData sd)
    {
        return NullBSDF();
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
