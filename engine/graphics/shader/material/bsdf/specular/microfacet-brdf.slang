// Specular microfacet BRDF and BSDF.
// Based on NVIDIA Falcor's SpecularMicrofacet.slang

import material.material_types;
import material.bsdf.math_constants;
import material.bsdf.math_helpers;
import material.bsdf.fresnel;
import material.bsdf.ggx;
import material.bsdf.bsdf_sample;
import material.bsdf.sample_generator;
import material.bsdf.i_bsdf;

// Specular reflection using microfacets with GGX distribution.
struct SpecularMicrofacetBRDF : IBSDF
{
    float3 albedo;      // Specular albedo (F0)
    float alpha;        // GGX width parameter
    uint activeLobes;   // BSDF lobes to include

    __init(float3 albedo_, float alpha_, uint activeLobes_)
    {
        albedo = albedo_;
        alpha = alpha_;
        activeLobes = activeLobes_;
    }

    bool hasLobe(LobeType lobeType)
    {
        return (activeLobes & (uint)lobeType) != 0;
    }

    // IBSDF interface
    float3 eval<S : ISampleGenerator>(float3 wi, float3 wo, inout S sg, BSDFContext bc)
    {
        if (min(wi.z, wo.z) < kMinCosTheta)
            return float3(0.0f);

        // Handle delta reflection
        if (alpha == 0.0f)
            return float3(0.0f);

        if (!hasLobe(LobeType::SpecularReflection))
            return float3(0.0f);

        float3 h = normalize(wi + wo);
        float wiDotH = dot(wi, h);

        float D = evalNdfGGX(alpha, h.z);
        float G = evalMaskingSmithGGXCorrelated(alpha, wi.z, wo.z);
        float3 F = evalFresnelSchlick(albedo, float3(1.0f), wiDotH);

        return F * D * G * 0.25f / wi.z;
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
        lobeType = (uint)LobeType::SpecularReflection;

        if (wi.z < kMinCosTheta)
            return false;

        // Handle delta reflection
        if (alpha == 0.0f)
        {
            if (!hasLobe(LobeType::DeltaReflection))
                return false;

            wo = float3(-wi.x, -wi.y, wi.z);
            pdf = 0.0f;
            weight = evalFresnelSchlick(albedo, float3(1.0f), wi.z);
            lobeType = (uint)LobeType::DeltaReflection;
            return true;
        }

        if (!hasLobe(LobeType::SpecularReflection))
            return false;

        // Sample the GGX VNDF
        float3 h = sampleGGX_VNDF(alpha, wi, sampleNext2D(sg), pdf);

        // Reflect the incident direction
        float wiDotH = dot(wi, h);
        wo = 2.0f * wiDotH * h - wi;

        if (wo.z < kMinCosTheta)
            return false;

        float G = evalMaskingSmithGGXCorrelated(alpha, wi.z, wo.z);
        float GOverG1wo = G * (1.0f + evalLambdaGGX(alpha * alpha, wi.z));
        float3 F = evalFresnelSchlick(albedo, float3(1.0f), wiDotH);

        pdf /= (4.0f * wiDotH);  // Jacobian of reflection operator
        weight = F * GOverG1wo;
        lobeType = (uint)LobeType::SpecularReflection;
        return true;
    }

    float evalPdf(float3 wi, float3 wo, BSDFContext bc)
    {
        if (min(wi.z, wo.z) < kMinCosTheta)
            return 0.0f;

        if (alpha == 0.0f)
            return 0.0f;

        if (!hasLobe(LobeType::SpecularReflection))
            return 0.0f;

        float3 h = normalize(wi + wo);
        float wiDotH = dot(wi, h);
        float pdf = evalPdfGGX_VNDF(alpha, wi, h);
        return pdf / (4.0f * wiDotH);
    }

    AlbedoContributions evalAlbedo(float3 wi, LobeType lobeType)
    {
        float3 r = evalFresnelSchlick(albedo, float3(1.0f), wi.z);
        return AlbedoContributions(r, float3(1.0f) - r, float3(0.0f), float3(0.0f));
    }

    float3 getIorAsReflectance()
    {
        return albedo;
    }

    RoughnessInformation getRoughnessInformation(float3 wi)
    {
        RoughnessInformation r;
        r.roughnessBSDFNotation = float2(alpha < kMinGGXAlpha ? 0.0f : alpha);
        return r;
    }
};

// Specular reflection and transmission using microfacets.
struct SpecularMicrofacetBSDF : IBSDF
{
    float3 transmissionAlbedo;  // Transmission albedo
    float alpha;                // GGX width parameter
    float eta;                  // Relative index of refraction (etaI / etaT)
    uint activeLobes;           // BSDF lobes to include

    __init(float3 transmissionAlbedo_, float alpha_, float eta_, uint activeLobes_)
    {
        transmissionAlbedo = transmissionAlbedo_;
        alpha = alpha_;
        eta = eta_;
        activeLobes = activeLobes_;
    }

    bool hasLobe(LobeType lobeType)
    {
        return (activeLobes & (uint)lobeType) != 0;
    }

    // IBSDF interface
    float3 eval<S : ISampleGenerator>(float3 wi, float3 wo, inout S sg, BSDFContext bc)
    {
        if (min(wi.z, abs(wo.z)) < kMinCosTheta)
            return float3(0.0f);

        // Handle delta reflection/transmission
        if (alpha == 0.0f)
            return float3(0.0f);

        bool hasReflection = hasLobe(LobeType::SpecularReflection);
        bool hasTransmission = hasLobe(LobeType::SpecularTransmission);
        bool isReflection = wo.z > 0.0f;

        if ((isReflection && !hasReflection) || (!isReflection && !hasTransmission))
            return float3(0.0f);

        // Compute half-vector and ensure it's in the upper hemisphere
        float3 h = normalize(wo + wi * (isReflection ? 1.0f : eta));
        h *= sign(h.z);

        float wiDotH = dot(wi, h);
        float woDotH = dot(wo, h);

        float D = evalNdfGGX(alpha, h.z);
        float G = evalMaskingSmithGGXCorrelated(alpha, wi.z, abs(wo.z));
        float F = evalFresnelDielectric(eta, wiDotH);

        if (isReflection)
        {
            return float3(F * D * G * 0.25f / wi.z);
        }
        else
        {
            float sqrtDenom = woDotH + eta * wiDotH;
            float t = eta * eta * wiDotH * woDotH / (wi.z * sqrtDenom * sqrtDenom);
            return transmissionAlbedo * (1.0f - F) * D * G * abs(t);
        }
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
        lobeType = (uint)LobeType::SpecularReflection;

        if (wi.z < kMinCosTheta)
            return false;

        float lobeSample = sampleNext1D(sg);

        // Handle delta reflection/transmission
        if (alpha == 0.0f)
        {
            bool hasReflection = hasLobe(LobeType::DeltaReflection);
            bool hasTransmission = hasLobe(LobeType::DeltaTransmission);

            if (!(hasReflection || hasTransmission))
                return false;

            float cosThetaT;
            float F = evalFresnelDielectric(eta, wi.z, cosThetaT);

            bool isReflection = hasReflection;
            if (hasReflection && hasTransmission)
            {
                isReflection = lobeSample < F;
            }
            else if (hasTransmission && F == 1.0f)
            {
                return false;
            }

            pdf = 0.0f;
            weight = isReflection ? float3(1.0f) : transmissionAlbedo;
            if (!(hasReflection && hasTransmission))
                weight *= isReflection ? F : (1.0f - F);

            wo = isReflection ?
                float3(-wi.x, -wi.y, wi.z) :
                float3(-wi.x * eta, -wi.y * eta, -cosThetaT);
            lobeType = isReflection ? (uint)LobeType::DeltaReflection : (uint)LobeType::DeltaTransmission;

            if (abs(wo.z) < kMinCosTheta || (wo.z > 0.0f != isReflection))
                return false;

            return true;
        }

        bool hasReflection = hasLobe(LobeType::SpecularReflection);
        bool hasTransmission = hasLobe(LobeType::SpecularTransmission);

        if (!(hasReflection || hasTransmission))
            return false;

        // Sample the GGX VNDF
        float3 h = sampleGGX_VNDF(alpha, wi, sampleNext2D(sg), pdf);
        float wiDotH = dot(wi, h);

        float cosThetaT;
        float F = evalFresnelDielectric(eta, wiDotH, cosThetaT);

        bool isReflection = hasReflection;
        if (hasReflection && hasTransmission)
        {
            isReflection = lobeSample < F;
        }
        else if (hasTransmission && F == 1.0f)
        {
            return false;
        }

        wo = isReflection ?
            (2.0f * wiDotH * h - wi) :
            ((eta * wiDotH - cosThetaT) * h - eta * wi);

        if (abs(wo.z) < kMinCosTheta || (wo.z > 0.0f != isReflection))
            return false;

        float woDotH = dot(wo, h);
        lobeType = isReflection ? (uint)LobeType::SpecularReflection : (uint)LobeType::SpecularTransmission;

        float G = evalMaskingSmithGGXCorrelated(alpha, wi.z, abs(wo.z));
        float GOverG1wo = G * (1.0f + evalLambdaGGX(alpha * alpha, wi.z));
        weight = float3(GOverG1wo);

        if (isReflection)
        {
            pdf /= 4.0f * woDotH;  // Jacobian of reflection operator
        }
        else
        {
            float sqrtDenom = woDotH + eta * wiDotH;
            float denom = sqrtDenom * sqrtDenom;
            pdf = (denom > 0.0f) ? pdf * abs(woDotH) / denom : FLT_MAX;
            weight *= transmissionAlbedo * eta * eta;
        }

        if (hasReflection && hasTransmission)
        {
            pdf *= isReflection ? F : (1.0f - F);
        }
        else
        {
            weight *= isReflection ? F : (1.0f - F);
        }

        return true;
    }

    float evalPdf(float3 wi, float3 wo, BSDFContext bc)
    {
        if (min(wi.z, abs(wo.z)) < kMinCosTheta)
            return 0.0f;

        if (alpha == 0.0f)
            return 0.0f;

        bool isReflection = wo.z > 0.0f;
        bool hasReflection = hasLobe(LobeType::SpecularReflection);
        bool hasTransmission = hasLobe(LobeType::SpecularTransmission);

        if ((isReflection && !hasReflection) || (!isReflection && !hasTransmission))
            return 0.0f;

        // Compute half-vector
        float3 h = normalize(wo + wi * (isReflection ? 1.0f : eta));
        h *= sign(h.z);

        float wiDotH = dot(wi, h);
        float woDotH = dot(wo, h);

        float F = evalFresnelDielectric(eta, wiDotH);
        float pdf = evalPdfGGX_VNDF(alpha, wi, h);

        if (isReflection)
        {
            pdf /= 4.0f * woDotH;
        }
        else
        {
            if (woDotH > 0.0f)
                return 0.0f;
            float sqrtDenom = woDotH + eta * wiDotH;
            float denom = sqrtDenom * sqrtDenom;
            pdf = (denom > 0.0f) ? pdf * abs(woDotH) / denom : FLT_MAX;
        }

        if (hasReflection && hasTransmission)
        {
            pdf *= isReflection ? F : (1.0f - F);
        }

        return pdf;
    }

    AlbedoContributions evalAlbedo(float3 wi, LobeType lobeType)
    {
        float3 r = float3(evalFresnelDielectric(eta, wi.z));
        return AlbedoContributions(float3(0.0f), float3(0.0f), float3(1.0f) - r, r);
    }

    float3 getIorAsReflectance()
    {
        float rsqrt = (1.0f - eta) / (1.0f + eta);
        return float3(rsqrt * rsqrt);
    }

    RoughnessInformation getRoughnessInformation(float3 wi)
    {
        RoughnessInformation r;
        r.roughnessBSDFNotation = float2(alpha);
        return r;
    }
};
