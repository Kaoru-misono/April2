// StandardMaterialInstance - per-shading-point standard BSDF instance.

import material.material_types;
import material.material_data;
import material.shading_data;
import material.i_material_instance;
import material.standard_bsdf;
import material.bsdf.sample_generator;

// Dummy sample generator for evaluation.
struct DummySampleGenerator : ISampleGenerator
{
    float next1D() { return 0.5f; }
    float2 next2D() { return float2(0.5f, 0.5f); }
};

// Sample generator from a float3 random tuple.
struct Float3SampleGenerator : ISampleGenerator
{
    float3 samples;
    int index;

    __init(float3 samples_)
    {
        samples = samples_;
        index = 0;
    }

    [mutating] float next1D()
    {
        float result = (index == 0) ? samples.x : ((index == 1) ? samples.y : samples.z);
        index = min(index + 1, 2);
        return result;
    }

    [mutating] float2 next2D()
    {
        float2 result;
        if (index == 0)
        {
            result = float2(samples.x, samples.y);
            index = 2;
        }
        else
        {
            result = float2(samples.y, samples.z);
            index = 3;
        }
        return result;
    }
};

struct StandardMaterialInstance : IMaterialInstance
{
    StandardMaterialData data;
    float3 sampledBaseColor;
    float3 sampledEmissive;
    float sampledMetallic;
    float sampledRoughness;

    __init(StandardMaterialData data_, float3 baseColor, float3 emissive, float metallic, float roughness)
    {
        data = data_;
        sampledBaseColor = baseColor;
        sampledEmissive = emissive;
        sampledMetallic = metallic;
        sampledRoughness = roughness;
    }

    MaterialType getType()
    {
        return MaterialType(data.header.materialType);
    }

    IBSDF getBSDF(ShadingData sd)
    {
        float3 wi = sd.toLocal(sd.V);

        StandardMaterialData mtlData = data;
        mtlData.metallic = sampledMetallic;
        mtlData.roughness = sampledRoughness;

        StandardBSDFData bsdfData = createStandardBSDFData(mtlData, sampledBaseColor, sd);
        return StandardBSDF(wi, data.header, bsdfData);
    }

    float3 eval(ShadingData sd, float3 wo)
    {
        float3 wi = sd.toLocal(sd.V);
        float3 woLocal = sd.toLocal(wo);
        IBSDF bsdf = getBSDF(sd);
        DummySampleGenerator sg;
        BSDFContext bc;
        return bsdf.eval(wi, woLocal, sg, bc);
    }

    float3 evalAD(DiffMaterialData diffData, ShadingData sd, float3 wo)
    {
        return eval(sd, wo);
    }

    bool sample(ShadingData sd, float3 u, out float3 wo, out float pdf, out float3 weight, out uint lobeType)
    {
        float3 wi = sd.toLocal(sd.V);
        IBSDF bsdf = getBSDF(sd);
        Float3SampleGenerator sg = Float3SampleGenerator(u);
        BSDFContext bc;
        float3 woLocal;
        bool valid = bsdf.sample(wi, woLocal, pdf, weight, lobeType, sg, bc);
        wo = sd.toWorld(woLocal);
        return valid;
    }

    float evalPdf(ShadingData sd, float3 wo)
    {
        float3 wi = sd.toLocal(sd.V);
        float3 woLocal = sd.toLocal(wo);
        IBSDF bsdf = getBSDF(sd);
        BSDFContext bc;
        return bsdf.evalPdf(wi, woLocal, bc);
    }

    BSDFProperties getProperties(ShadingData sd)
    {
        BSDFProperties p;
        p.emission = sampledEmissive;
        p.roughness = sampledRoughness;
        p.guideNormal = sd.N;
        p.flags = 0;
        return p;
    }

    uint getLobeTypes(ShadingData sd)
    {
        StandardMaterialData mtlData = data;
        mtlData.metallic = sampledMetallic;
        mtlData.roughness = sampledRoughness;
        StandardBSDFData bsdfData = createStandardBSDFData(mtlData, sampledBaseColor, sd);
        return StandardBSDF::getLobeTypes(bsdfData);
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
        return sampledEmissive;
    }

    float3 getAlbedo(ShadingData sd)
    {
        return lerp(sampledBaseColor, float3(0.0f), sampledMetallic);
    }

    float getRoughness(ShadingData sd)
    {
        return sampledRoughness;
    }
};
