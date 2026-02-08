// Fresnel functions for material rendering.
// Based on NVIDIA Falcor's Fresnel.slang

import material.bsdf.math_helpers;

// Evaluates the Fresnel term using Schlick's approximation.
// The Fresnel term equals f0 at normal incidence, and approaches f90=1.0 at 90 degrees.
//
// @param f0 Specular reflectance at normal incidence (0 degrees).
// @param f90 Reflectance at orthogonal incidence (90 degrees).
// @param cosTheta Cosine of angle between microfacet normal and incident direction.
// @return Fresnel term.
float3 evalFresnelSchlick(float3 f0, float3 f90, float cosTheta)
{
    return f0 + (f90 - f0) * pow(max(1.0f - cosTheta, 0.0f), 5.0f);
}

float evalFresnelSchlick(float f0, float f90, float cosTheta)
{
    return f0 + (f90 - f0) * pow(max(1.0f - cosTheta, 0.0f), 5.0f);
}

// Convenience overload with f90 = 1.0
float3 evalFresnelSchlick(float3 f0, float cosTheta)
{
    return evalFresnelSchlick(f0, float3(1.0f), cosTheta);
}

// Generalized Schlick Fresnel with custom exponent
float3 evalFresnelGeneralizedSchlick(float3 f0, float3 f90, float exponent, float cosTheta)
{
    return f0 + (f90 - f0) * pow(max(1.0f - cosTheta, 0.0f), exponent);
}

// Evaluates the Fresnel term using dielectric Fresnel equations.
// Based on PBRT's implementation.
//
// @param eta Relative index of refraction (etaI / etaT).
// @param cosThetaI Cosine of angle between normal and incident direction.
// @param cosThetaT Output: cosine of angle between negative normal and transmitted direction.
// @return Fresnel reflectance Fr(eta, cosThetaI).
float evalFresnelDielectric(float eta, float cosThetaI, out float cosThetaT)
{
    if (cosThetaI < 0.0f)
    {
        eta = 1.0f / eta;
        cosThetaI = -cosThetaI;
    }

    float sinThetaTSq = eta * eta * (1.0f - cosThetaI * cosThetaI);

    // Check for total internal reflection
    if (sinThetaTSq > 1.0f)
    {
        cosThetaT = 0.0f;
        return 1.0f;
    }

    cosThetaT = sqrt(1.0f - sinThetaTSq);

    float Rs = (eta * cosThetaI - cosThetaT) / (eta * cosThetaI + cosThetaT);
    float Rp = (eta * cosThetaT - cosThetaI) / (eta * cosThetaT + cosThetaI);

    return 0.5f * (Rs * Rs + Rp * Rp);
}

// Overload without transmitted angle output
float evalFresnelDielectric(float eta, float cosThetaI)
{
    float cosThetaT;
    return evalFresnelDielectric(eta, cosThetaI, cosThetaT);
}

// Evaluates the Fresnel term for conductors (metals).
// Based on "Physically Based Lighting Calculations for Computer Graphics" by Peter Shirley.
//
// @param eta Real part of complex index of refraction.
// @param k Imaginary part (absorption coefficient).
// @param cosThetaI Cosine of angle between normal and incident direction.
// @return Conductor reflectance.
float evalFresnelConductor(float eta, float k, float cosThetaI)
{
    float cosThetaISq = cosThetaI * cosThetaI;
    float sinThetaISq = max(1.0f - cosThetaISq, 0.0f);
    float sinThetaIQu = sinThetaISq * sinThetaISq;

    float innerTerm = eta * eta - k * k - sinThetaISq;
    float aSqPlusBSq = sqrt(max(innerTerm * innerTerm + 4.0f * eta * eta * k * k, 0.0f));
    float a = sqrt(max((aSqPlusBSq + innerTerm) * 0.5f, 0.0f));

    float Rs = ((aSqPlusBSq + cosThetaISq) - (2.0f * a * cosThetaI)) /
               ((aSqPlusBSq + cosThetaISq) + (2.0f * a * cosThetaI));
    float Rp = ((cosThetaISq * aSqPlusBSq + sinThetaIQu) - (2.0f * a * cosThetaI * sinThetaISq)) /
               ((cosThetaISq * aSqPlusBSq + sinThetaIQu) + (2.0f * a * cosThetaI * sinThetaISq));

    return 0.5f * (Rs + Rs * Rp);
}

// Conductor Fresnel for 3 wavelengths (RGB)
float3 evalFresnelConductor(float3 eta, float3 k, float cosThetaI)
{
    return float3(
        evalFresnelConductor(eta.x, k.x, cosThetaI),
        evalFresnelConductor(eta.y, k.y, cosThetaI),
        evalFresnelConductor(eta.z, k.z, cosThetaI)
    );
}

// Converts reflectance at normal incidence to IOR.
// Assumes n_i = 1.0 (air).
float reflectanceToIOR(float3 reflectance)
{
    float r = min(luminance(reflectance), 0.999f);
    float iorI = 1.0f;
    return (-iorI - 2.0f * iorI * sqrt(r) - iorI * r) / (r - 1.0f);
}
