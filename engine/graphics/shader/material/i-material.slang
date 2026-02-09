// IMaterial interface - High-level material interface.

import material.material_types;
import material.material_data;
import material.shading_data;
import material.bsdf.i_bsdf;

// Material instance interface for runtime material evaluation.
interface IMaterialInstance
{
    // Get the type of this material.
    MaterialType getType();

    // Create a BSDF interface instance for this shading point.
    IBSDF getBSDF(ShadingData sd);

    // Evaluate the BSDF at a shading point.
    // Returns f(wi, wo) * dot(wo, n) in world space.
    float3 eval(ShadingData sd, float3 wo);

    // Sample the BSDF at a shading point.
    // @param sd Shading data.
    // @param u Random numbers for sampling.
    // @param wo Output: sampled outgoing direction (world space).
    // @param pdf Output: sampling pdf.
    // @param weight Output: sample weight.
    // @param lobeType Output: sampled lobe type.
    // @return True if successful.
    bool sample(ShadingData sd, float3 u, out float3 wo, out float pdf, out float3 weight, out uint lobeType);

    // Evaluate the pdf for a given outgoing direction.
    float evalPdf(ShadingData sd, float3 wo);

    // Get the emission from this material.
    float3 getEmission(ShadingData sd);

    // Get material properties for denoising/debugging.
    float3 getAlbedo(ShadingData sd);
    float getRoughness(ShadingData sd);
};
