// StandardBSDF - Mixed BSDF for standard PBR materials.
// Based on NVIDIA Falcor's StandardBSDF.slang

import material.material_types;
import material.material_data;
import material.bsdf.math_constants;
import material.bsdf.math_helpers;
import material.bsdf.fresnel;
import material.bsdf.bsdf_sample;
import material.bsdf.sample_generator;
import material.bsdf.i_bsdf;
import material.bsdf.diffuse.lambert_brdf;
import material.bsdf.diffuse.frostbite_brdf;
import material.bsdf.diffuse.lambert_btdf;
import material.bsdf.specular.microfacet_brdf;

// BSDF parameters for the standard material.
struct StandardBSDFData
{
    float3 diffuse;             // Diffuse albedo
    float3 specular;            // Specular albedo (F0)
    float roughness;            // Linear roughness (before remapping)
    float metallic;             // Metallic parameter
    float eta;                  // Relative index of refraction
    float3 transmission;        // Transmission color
    float diffuseTransmission;  // Blend between diffuse BRDF and BTDF
    float specularTransmission; // Blend between opaque and transmissive BSDF
};

// Create StandardBSDFData from StandardMaterialData.
StandardBSDFData createStandardBSDFData(StandardMaterialData mtl, float3 baseColor)
{
    StandardBSDFData data;

    // Derive diffuse and specular from metallic workflow
    float3 dielectricF0 = float3(0.04f);  // Default F0 for dielectrics
    data.specular = lerp(dielectricF0, baseColor, mtl.metallic);
    data.diffuse = baseColor * (1.0f - mtl.metallic);

    data.roughness = mtl.roughness;
    data.metallic = mtl.metallic;
    data.eta = 1.0f / mtl.ior;  // eta = etaI / etaT, assuming air outside
    data.transmission = mtl.transmission;
    data.diffuseTransmission = mtl.diffuseTransmission;
    data.specularTransmission = mtl.specularTransmission;

    return data;
}

// Mixed BSDF for standard PBR materials.
// Combines diffuse and specular reflection with optional transmission.
struct StandardBSDF : IBSDF
{
    // Use Frostbite diffuse (can be swapped via type conformance in the future)
    FrostbiteDiffuseBRDF diffuseReflection;
    LambertDiffuseBTDF diffuseTransmission;

    SpecularMicrofacetBRDF specularReflection;
    SpecularMicrofacetBSDF specularTransmission;

    float diffTrans;    // Mix between diffuse BRDF and diffuse BTDF
    float specTrans;    // Mix between dielectric BRDF and specular BSDF

    // Sampling probabilities (not differentiable)
    float pDiffuseReflection;
    float pDiffuseTransmission;
    float pSpecularReflection;
    float pSpecularTransmission;

    // Initialize from BSDF data.
    __init(float3 wi, MaterialHeader mtl, StandardBSDFData data)
    {
        // Use square root for transmission albedo if not thin surface
        float3 transmissionAlbedo = data.transmission;

        // Setup lobes
        diffuseReflection = FrostbiteDiffuseBRDF(data.diffuse, data.roughness);
        diffuseTransmission = LambertDiffuseBTDF(transmissionAlbedo);

        // Compute GGX alpha
        float alpha = data.roughness * data.roughness;
        if (alpha < kMinGGXAlpha)
            alpha = 0.0f;  // Use delta distribution

        uint activeLobes = mtl.activeLobes;

        specularReflection = SpecularMicrofacetBRDF(data.specular, alpha, activeLobes);

        // Transmission through rough interface with same IOR is not well defined
        float transmissionAlpha = (data.eta == 1.0f) ? 0.0f : alpha;
        specularTransmission = SpecularMicrofacetBSDF(transmissionAlbedo, transmissionAlpha, data.eta, activeLobes);

        diffTrans = data.diffuseTransmission;
        specTrans = data.specularTransmission;

        // Compute sampling weights
        float metallicBRDF = data.metallic * (1.0f - specTrans);
        float dielectricBSDF = (1.0f - data.metallic) * (1.0f - specTrans);
        float specularBSDF = specTrans;

        float diffuseWeight = luminance(data.diffuse);
        float specularWeight = luminance(evalFresnelSchlick(data.specular, float3(1.0f), wi.z));

        pDiffuseReflection = ((activeLobes & (uint)LobeType::DiffuseReflection) != 0) ?
            diffuseWeight * dielectricBSDF * (1.0f - diffTrans) : 0.0f;
        pDiffuseTransmission = ((activeLobes & (uint)LobeType::DiffuseTransmission) != 0) ?
            diffuseWeight * dielectricBSDF * diffTrans : 0.0f;
        pSpecularReflection = ((activeLobes & ((uint)LobeType::SpecularReflection | (uint)LobeType::DeltaReflection)) != 0) ?
            specularWeight * (metallicBRDF + dielectricBSDF) : 0.0f;
        pSpecularTransmission = ((activeLobes & ((uint)LobeType::SpecularReflection | (uint)LobeType::DeltaReflection |
            (uint)LobeType::SpecularTransmission | (uint)LobeType::DeltaTransmission)) != 0) ?
            specularBSDF : 0.0f;

        float normFactor = pDiffuseReflection + pDiffuseTransmission + pSpecularReflection + pSpecularTransmission;
        if (normFactor > 0.0f)
        {
            normFactor = 1.0f / normFactor;
            pDiffuseReflection *= normFactor;
            pDiffuseTransmission *= normFactor;
            pSpecularReflection *= normFactor;
            pSpecularTransmission *= normFactor;
        }
    }

    // Get the lobe types this BSDF supports.
    static uint getLobeTypes(StandardBSDFData data)
    {
        float alpha = data.roughness * data.roughness;
        bool isDelta = alpha < kMinGGXAlpha;

        float diffTrans = data.diffuseTransmission;
        float specTrans = data.specularTransmission;

        uint lobeTypes = isDelta ? (uint)LobeType::DeltaReflection : (uint)LobeType::SpecularReflection;

        if (any(data.diffuse > float3(0.0f)) && specTrans < 1.0f)
        {
            if (diffTrans < 1.0f)
                lobeTypes |= (uint)LobeType::DiffuseReflection;
            if (diffTrans > 0.0f)
                lobeTypes |= (uint)LobeType::DiffuseTransmission;
        }

        if (specTrans > 0.0f)
            lobeTypes |= isDelta ? (uint)LobeType::DeltaTransmission : (uint)LobeType::SpecularTransmission;

        return lobeTypes;
    }

    // IBSDF interface
    float3 eval<S : ISampleGenerator>(float3 wi, float3 wo, inout S sg, BSDFContext bc)
    {
        float3 result = float3(0.0f);

        if (pDiffuseReflection > 0.0f)
            result += (1.0f - specTrans) * (1.0f - diffTrans) * diffuseReflection.eval(wi, wo, sg, bc);
        if (pDiffuseTransmission > 0.0f)
            result += (1.0f - specTrans) * diffTrans * diffuseTransmission.eval(wi, wo, sg, bc);
        if (pSpecularReflection > 0.0f)
            result += (1.0f - specTrans) * specularReflection.eval(wi, wo, sg, bc);
        if (pSpecularTransmission > 0.0f)
            result += specTrans * specularTransmission.eval(wi, wo, sg, bc);

        return result;
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
        weight = float3(0.0f);
        pdf = 0.0f;
        lobeType = (uint)LobeType::DiffuseReflection;

        bool valid = false;
        float uSelect = sampleNext1D(sg);

        if (uSelect < pDiffuseReflection)
        {
            valid = diffuseReflection.sample(wi, wo, pdf, weight, lobeType, sg, bc);
            weight /= pDiffuseReflection;
            weight *= (1.0f - specTrans) * (1.0f - diffTrans);
            pdf *= pDiffuseReflection;

            if (pSpecularReflection > 0.0f)
                pdf += pSpecularReflection * specularReflection.evalPdf(wi, wo, bc);
            if (pSpecularTransmission > 0.0f)
                pdf += pSpecularTransmission * specularTransmission.evalPdf(wi, wo, bc);
        }
        else if (uSelect < pDiffuseReflection + pDiffuseTransmission)
        {
            valid = diffuseTransmission.sample(wi, wo, pdf, weight, lobeType, sg, bc);
            weight /= pDiffuseTransmission;
            weight *= (1.0f - specTrans) * diffTrans;
            pdf *= pDiffuseTransmission;

            if (pSpecularTransmission > 0.0f)
                pdf += pSpecularTransmission * specularTransmission.evalPdf(wi, wo, bc);
        }
        else if (uSelect < pDiffuseReflection + pDiffuseTransmission + pSpecularReflection)
        {
            valid = specularReflection.sample(wi, wo, pdf, weight, lobeType, sg, bc);
            weight /= pSpecularReflection;
            weight *= (1.0f - specTrans);
            pdf *= pSpecularReflection;

            if (pDiffuseReflection > 0.0f)
                pdf += pDiffuseReflection * diffuseReflection.evalPdf(wi, wo, bc);
            if (pSpecularTransmission > 0.0f)
                pdf += pSpecularTransmission * specularTransmission.evalPdf(wi, wo, bc);
        }
        else if (pSpecularTransmission > 0.0f)
        {
            valid = specularTransmission.sample(wi, wo, pdf, weight, lobeType, sg, bc);
            weight /= pSpecularTransmission;
            weight *= specTrans;
            pdf *= pSpecularTransmission;

            if (pDiffuseReflection > 0.0f)
                pdf += pDiffuseReflection * diffuseReflection.evalPdf(wi, wo, bc);
            if (pDiffuseTransmission > 0.0f)
                pdf += pDiffuseTransmission * diffuseTransmission.evalPdf(wi, wo, bc);
            if (pSpecularReflection > 0.0f)
                pdf += pSpecularReflection * specularReflection.evalPdf(wi, wo, bc);
        }

        return valid;
    }

    float evalPdf(float3 wi, float3 wo, BSDFContext bc)
    {
        float pdf = 0.0f;

        if (pDiffuseReflection > 0.0f)
            pdf += pDiffuseReflection * diffuseReflection.evalPdf(wi, wo, bc);
        if (pDiffuseTransmission > 0.0f)
            pdf += pDiffuseTransmission * diffuseTransmission.evalPdf(wi, wo, bc);
        if (pSpecularReflection > 0.0f)
            pdf += pSpecularReflection * specularReflection.evalPdf(wi, wo, bc);
        if (pSpecularTransmission > 0.0f)
            pdf += pSpecularTransmission * specularTransmission.evalPdf(wi, wo, bc);

        return pdf;
    }

    AlbedoContributions evalAlbedo(float3 wi, LobeType lobeType)
    {
        AlbedoContributions a;

        if (pDiffuseReflection > 0.0f)
            a = a + (1.0f - specTrans) * (1.0f - diffTrans) * diffuseReflection.evalAlbedo(wi, lobeType);
        if (pDiffuseTransmission > 0.0f)
            a = a + (1.0f - specTrans) * diffTrans * diffuseTransmission.evalAlbedo(wi, lobeType);
        if (pSpecularReflection > 0.0f)
            a = a + (1.0f - specTrans) * specularReflection.evalAlbedo(wi, lobeType);
        if (pSpecularTransmission > 0.0f)
            a = a + specTrans * specularTransmission.evalAlbedo(wi, lobeType);

        return a;
    }

    float3 getIorAsReflectance()
    {
        return float3(1.0f);
    }

    RoughnessInformation getRoughnessInformation(float3 wi)
    {
        RoughnessInformation r;
        float alpha = specularReflection.alpha;
        r.roughnessBSDFNotation = float2(alpha < kMinGGXAlpha ? 0.0f : alpha);
        return r;
    }
};
