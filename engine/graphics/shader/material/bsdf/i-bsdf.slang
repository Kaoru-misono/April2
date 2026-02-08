// IBSDF interface - Low-level interface for BSDF functions.
// Based on NVIDIA Falcor's IBSDF.slang

import material.material_types;
import material.bsdf.math_constants;
import material.bsdf.bsdf_sample;
import material.bsdf.sample_generator;

// Low-level interface for BSDF functions.
//
// Conventions:
// - All operations are done in a local coordinate frame.
// - The local frame has normal N=(0,0,1), tangent T=(1,0,0) and bitangent B=(0,1,0).
// - The incident and outgoing direction point away from the shading location.
// - The incident direction (wi) is always in the positive hemisphere.
// - The outgoing direction (wo) is sampled.
// - Evaluating the BSDF always includes the foreshortening term (dot(wo, n) = wo.z).
[anyValueSize(96)]
interface IBSDF
{
    // Evaluates the BSDF.
    // @param wi Incident direction.
    // @param wo Outgoing direction.
    // @param sg Sample generator (for stochastic evaluation if needed).
    // @param bc BSDF context with IOR information.
    // @return f(wi, wo) * dot(wo, n)
    float3 eval<S : ISampleGenerator>(float3 wi, float3 wo, inout S sg, BSDFContext bc);

    // Samples the BSDF.
    // @param wi Incident direction.
    // @param wo Output: sampled outgoing direction.
    // @param pdf Output: pdf with respect to solid angle (0 if delta event).
    // @param weight Output: sample weight f(wi, wo) * dot(wo, n) / pdf(wo).
    // @param lobeType Output: sampled lobe type (see LobeType).
    // @param sg Sample generator.
    // @param bc BSDF context.
    // @return True if successful.
    bool sample<S : ISampleGenerator>(
        float3 wi,
        out float3 wo,
        out float pdf,
        out float3 weight,
        out uint lobeType,
        inout S sg,
        BSDFContext bc
    );

    // Evaluates the directional pdf for sampling outgoing direction wo.
    // @param wi Incident direction.
    // @param wo Outgoing direction.
    // @param bc BSDF context.
    // @return PDF with respect to solid angle (0 for delta events).
    float evalPdf(float3 wi, float3 wo, BSDFContext bc);

    // Evaluates the albedo (hemispherical reflectance).
    // @param wi Incident direction.
    // @param lobeType Lobe types to evaluate.
    // @return Albedo contributions.
    AlbedoContributions evalAlbedo(float3 wi, LobeType lobeType);

    // Returns unattenuated reflectance at normal incidence (for IOR conversion).
    float3 getIorAsReflectance();

    // Returns information about the roughness of the BSDF.
    // @param wi Incident direction.
    RoughnessInformation getRoughnessInformation(float3 wi);
};
