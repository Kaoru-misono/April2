// Frostbite diffuse BRDF.
// Disney's diffuse BRDF with energy conservation normalization.
// Based on "Moving Frostbite to PBR" by Lagarde and de Rousiers.

import material.material_types;
import material.bsdf.math_constants;
import material.bsdf.math_helpers;
import material.bsdf.fresnel;
import material.bsdf.bsdf_sample;
import material.bsdf.sample_generator;
import material.bsdf.i_bsdf;
import material.bsdf.diffuse.i_diffuse_brdf;

// Frostbite diffuse BRDF with roughness-dependent energy conservation.
struct FrostbiteDiffuseBRDF : IBSDF, IDiffuseBRDF
{
    float3 albedo;      // Diffuse albedo
    float roughness;    // Roughness before remapping

    __init(float3 albedo_, float roughness_)
    {
        albedo = albedo_;
        roughness = roughness_;
    }

    // Internal: evaluate weight f(wi, wo) * pi
    float3 evalWeight(float3 wi, float3 wo)
    {
        float3 h = normalize(wi + wo);
        float woDotH = dot(wo, h);

        float energyBias = lerp(0.0f, 0.5f, roughness);
        float energyFactor = lerp(1.0f, 1.0f / 1.51f, roughness);

        float fd90 = energyBias + 2.0f * woDotH * woDotH * roughness;
        float fd0 = 1.0f;

        float wiScatter = evalFresnelSchlick(fd0, fd90, wi.z);
        float woScatter = evalFresnelSchlick(fd0, fd90, wo.z);

        return albedo * wiScatter * woScatter * energyFactor;
    }

    // IDiffuseBRDF interface
    float3 eval(float3 wi, float3 wo)
    {
        if (min(wi.z, wo.z) < kMinCosTheta)
            return float3(0.0f);
        return evalWeight(wi, wo) * M_1_PI * wo.z;
    }

    bool sample(float3 wi, float2 u, out float3 wo, out float pdf, out float3 weight)
    {
        wo = sample_cosine_hemisphere_concentric(u, pdf);

        if (min(wi.z, wo.z) < kMinCosTheta)
        {
            weight = float3(0.0f);
            return false;
        }

        weight = evalWeight(wi, wo);
        return true;
    }

    float evalPdf(float3 wi, float3 wo)
    {
        if (min(wi.z, wo.z) < kMinCosTheta)
            return 0.0f;
        return M_1_PI * wo.z;
    }

    float3 getAlbedo()
    {
        return albedo;
    }

    // IBSDF interface
    float3 eval<S : ISampleGenerator>(float3 wi, float3 wo, inout S sg, BSDFContext bc)
    {
        return eval(wi, wo);
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
        lobeType = (uint)LobeType::DiffuseReflection;
        return sample(wi, sampleNext2D(sg), wo, pdf, weight);
    }

    float evalPdf(float3 wi, float3 wo, BSDFContext bc)
    {
        return evalPdf(wi, wo);
    }

    AlbedoContributions evalAlbedo(float3 wi, LobeType lobeType)
    {
        return AlbedoContributions(albedo, float3(1.0f) - albedo, float3(0.0f), float3(0.0f));
    }

    float3 getIorAsReflectance()
    {
        return float3(1.0f);
    }

    RoughnessInformation getRoughnessInformation(float3 wi)
    {
        RoughnessInformation r;
        r.roughnessBSDFNotation = float2(0.5f, 0.5f);
        return r;
    }
};
