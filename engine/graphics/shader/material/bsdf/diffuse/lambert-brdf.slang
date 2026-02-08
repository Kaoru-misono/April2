// Lambertian diffuse BRDF.
// f_r(wi, wo) = albedo / pi

import material.material_types;
import material.bsdf.math_constants;
import material.bsdf.math_helpers;
import material.bsdf.bsdf_sample;
import material.bsdf.sample_generator;
import material.bsdf.i_bsdf;
import material.bsdf.diffuse.i_diffuse_brdf;

// Lambertian diffuse reflection BRDF.
struct LambertDiffuseBRDF : IBSDF, IDiffuseBRDF
{
    float3 albedo;  // Diffuse albedo

    __init(float3 albedo_)
    {
        albedo = albedo_;
    }

    // IDiffuseBRDF interface
    float3 eval(float3 wi, float3 wo)
    {
        if (min(wi.z, wo.z) < kMinCosTheta)
            return float3(0.0f);
        return M_1_PI * albedo * wo.z;
    }

    bool sample(float3 wi, float2 u, out float3 wo, out float pdf, out float3 weight)
    {
        wo = sample_cosine_hemisphere_concentric(u, pdf);

        if (min(wi.z, wo.z) < kMinCosTheta)
        {
            weight = float3(0.0f);
            return false;
        }

        weight = albedo;
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
