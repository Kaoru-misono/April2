// StandardMaterial - Standard PBR material implementation.

import material.material_types;
import material.material_data;
import material.shading_data;
import material.i_material;
import material.standard_bsdf;
import material.bsdf.math_helpers;
import material.bsdf.bsdf_sample;
import material.bsdf.sample_generator;

// Standard PBR material instance.
struct StandardMaterialInstance : IMaterialInstance
{
    StandardMaterialData data;
    float3 sampledBaseColor;    // Base color after texture sampling
    float3 sampledEmissive;     // Emissive after texture sampling
    float sampledMetallic;      // Metallic after texture sampling
    float sampledRoughness;     // Roughness after texture sampling

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

        StandardBSDFData bsdfData = createStandardBSDFData(mtlData, sampledBaseColor);
        return StandardBSDF(wi, data.header, bsdfData);
    }

    float3 eval(ShadingData sd, float3 wo)
    {
        float3 wi = sd.toLocal(sd.V);
        float3 woLocal = sd.toLocal(wo);

        IBSDF bsdf = getBSDF(sd);

        // Create a dummy sample generator for eval (not used for non-stochastic evaluation)
        DummySampleGenerator sg;
        BSDFContext bc;
        return bsdf.eval(wi, woLocal, sg, bc);
    }

    bool sample(ShadingData sd, float3 u, out float3 wo, out float pdf, out float3 weight, out uint lobeType)
    {
        float3 wi = sd.toLocal(sd.V);

        IBSDF bsdf = getBSDF(sd);

        // Create sample generator from provided random numbers
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

    float3 getEmission(ShadingData sd)
    {
        return sampledEmissive;
    }

    float3 getAlbedo(ShadingData sd)
    {
        // For denoising, return a simple approximation of albedo
        return lerp(sampledBaseColor, float3(0.0f), sampledMetallic);
    }

    float getRoughness(ShadingData sd)
    {
        return sampledRoughness;
    }
};

// Dummy sample generator for evaluation (doesn't consume samples)
struct DummySampleGenerator : ISampleGenerator
{
    float next1D() { return 0.5f; }
    float2 next2D() { return float2(0.5f, 0.5f); }
};

// Sample generator from a float3 of random numbers
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
