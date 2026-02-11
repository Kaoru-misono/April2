// StandardMaterialInstance - per-shading-point standard BSDF instance.

import material.material_types;
import material.material_data;
import material.basic_material_data;
import material.shading_data;
import material.i_material_instance;
import material.standard_bsdf;
import material.bsdf.i_bsdf;
import material.bsdf.bsdf_sample;
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

struct StandardMaterialInstance : MaterialInstanceBase, IMaterialInstance
{
    MaterialHeader header;
    BasicMaterialData data;
    float3 sampledBaseColor;
    float3 sampledEmissive;
    float sampledMetallic;
    float sampledRoughness;

    __init(MaterialHeader header_, BasicMaterialData data_, float3 baseColor, float3 emissive, float metallic, float roughness)
    {
        header = header_;
        data = data_;
        sampledBaseColor = baseColor;
        sampledEmissive = emissive;
        sampledMetallic = metallic;
        sampledRoughness = roughness;
    }

    IBSDF getBSDF(ShadingData sd)
    {
        float3 wi = sd.toLocal(sd.V);

        BasicMaterialData mtlData = data;
        mtlData.specular.g = sampledRoughness;
        mtlData.specular.b = sampledMetallic;

        StandardBSDFData bsdfData = createStandardBSDFData(mtlData, sampledBaseColor, sd);
        return StandardBSDF(wi, header, bsdfData);
    }

    float3 eval<S : ISampleGenerator>(ShadingData sd, float3 wo, inout S sg)
    {
        float3 wi = sd.toLocal(sd.V);
        float3 woLocal = sd.toLocal(wo);
        IBSDF bsdf = getBSDF(sd);
        BSDFContext bc;
        return bsdf.eval(wi, woLocal, sg, bc);
    }

    float3 evalAD<S : ISampleGenerator>(DiffMaterialData diffData, ShadingData sd, float3 wo, inout S sg)
    {
        return eval(sd, wo, sg);
    }

    bool sample<S : ISampleGenerator>(ShadingData sd, inout S sg, out BSDFSample result, bool useImportanceSampling = true)
    {
        float3 wi = sd.toLocal(sd.V);
        IBSDF bsdf = getBSDF(sd);
        BSDFContext bc;
        float3 woLocal = float3(0.0f);
        result.pdf = 0.0f;
        result.weight = float3(0.0f);
        result.lobeType = (uint)LobeType::None;
        bool valid = bsdf.sample(wi, woLocal, result.pdf, result.weight, result.lobeType, sg, bc);
        result.wo = sd.toWorld(woLocal);
        return valid;
    }

    float evalPdf(ShadingData sd, float3 wo, bool useImportanceSampling = true)
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
        p.diffuseReflectionAlbedo = lerp(sampledBaseColor, float3(0.0f), sampledMetallic);
        p.diffuseTransmissionAlbedo = float3(0.0f);
        p.specularReflectionAlbedo = lerp(float3(0.04f), sampledBaseColor, sampledMetallic);
        p.specularTransmissionAlbedo = float3(data.specularTransmission) * data.transmission;
        p.specularReflectance = p.specularReflectionAlbedo;
        p.flags = data.specularTransmission > 0.0f || data.diffuseTransmission > 0.0f ? (uint)BSDFProperties::Flags::IsTransmissive : 0u;
        return p;
    }

    uint getLobeTypes(ShadingData sd)
    {
        BasicMaterialData mtlData = data;
        mtlData.specular.g = sampledRoughness;
        mtlData.specular.b = sampledMetallic;
        StandardBSDFData bsdfData = createStandardBSDFData(mtlData, sampledBaseColor, sd);
        return StandardBSDF::getLobeTypes(bsdfData);
    }

};
