// Lambertian diffuse transmission BTDF.

import material.material_types;
import material.bsdf.math_constants;
import material.bsdf.math_helpers;
import material.bsdf.bsdf_sample;
import material.bsdf.sample_generator;
import material.bsdf.i_bsdf;

// Lambertian diffuse transmission.
struct LambertDiffuseBTDF : IBSDF
{
    float3 albedo;  // Transmission albedo

    __init(float3 albedo_)
    {
        albedo = albedo_;
    }

    // IBSDF interface
    float3 eval<S : ISampleGenerator>(float3 wi, float3 wo, inout S sg, BSDFContext bc)
    {
        if (min(wi.z, -wo.z) < kMinCosTheta)
            return float3(0.0f);
        return M_1_PI * albedo * -wo.z;
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
        wo = sample_cosine_hemisphere_concentric(sampleNext2D(sg), pdf);
        wo.z = -wo.z;  // Flip to lower hemisphere
        lobeType = (uint)LobeType::DiffuseTransmission;

        if (min(wi.z, -wo.z) < kMinCosTheta)
        {
            weight = float3(0.0f);
            return false;
        }

        weight = albedo;
        return true;
    }

    float evalPdf(float3 wi, float3 wo, BSDFContext bc)
    {
        if (min(wi.z, -wo.z) < kMinCosTheta)
            return 0.0f;
        return M_1_PI * -wo.z;
    }

    AlbedoContributions evalAlbedo(float3 wi, LobeType lobeType)
    {
        return AlbedoContributions(float3(0.0f), float3(0.0f), albedo, float3(1.0f) - albedo);
    }

    float3 getIorAsReflectance()
    {
        return float3(0.0f);
    }

    RoughnessInformation getRoughnessInformation(float3 wi)
    {
        RoughnessInformation r;
        r.roughnessBSDFNotation = float2(0.5f, 0.5f);
        return r;
    }
};
