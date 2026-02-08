// Interface for diffuse BRDF implementations.
// Allows switching between Lambert, Frostbite, etc. via type conformances.

import material.material_types;
import material.bsdf.math_constants;
import material.bsdf.bsdf_sample;
import material.bsdf.sample_generator;

// Interface for diffuse BRDF implementations.
// All implementations should conform to this interface.
interface IDiffuseBRDF
{
    // Evaluates the diffuse BRDF.
    // @param wi Incident direction in local frame.
    // @param wo Outgoing direction in local frame.
    // @return f(wi, wo) * dot(wo, n)
    float3 eval(float3 wi, float3 wo);

    // Samples the diffuse BRDF (cosine-weighted hemisphere sampling).
    // @param wi Incident direction.
    // @param u Random numbers for sampling.
    // @param wo Output: sampled outgoing direction.
    // @param pdf Output: sampling pdf.
    // @param weight Output: sample weight.
    // @return True if successful.
    bool sample(float3 wi, float2 u, out float3 wo, out float pdf, out float3 weight);

    // Evaluates the pdf for the sampled direction.
    // @param wi Incident direction.
    // @param wo Outgoing direction.
    // @return PDF with respect to solid angle.
    float evalPdf(float3 wi, float3 wo);

    // Returns the diffuse albedo.
    float3 getAlbedo();
};
