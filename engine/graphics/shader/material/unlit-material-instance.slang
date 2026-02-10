import material.material_types;
import material.material_data;
import material.shading_data;
import material.i_material_instance;
import material.material_types;
import material.bsdf.i_bsdf;
import material.bsdf.bsdf_sample;
import material.bsdf.sample_generator;

struct NullBSDF : IBSDF
{
    __init()
    {
    }

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

    float3 evalAD(DiffMaterialData diffData, ShadingData sd, float3 wo)
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

    BSDFProperties getProperties(ShadingData sd)
    {
        BSDFProperties p;
        p.emission = data.emissive;
        p.roughness = 1.0f;
        p.albedo = data.baseColor.rgb;
        p.flags = 0;
        return p;
    }

    uint getLobeTypes(ShadingData sd)
    {
        return (uint)LobeType::None;
    }

    bool hasVolumeProperties()
    {
        return false;
    }

    VolumeProperties getVolumeProperties()
    {
        VolumeProperties p;
        p.sigmaA = 0.0f;
        p.sigmaS = 0.0f;
        p.g = 0.0f;
        return p;
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
