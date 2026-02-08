// GGX (Trowbridge-Reitz) normal distribution and related functions.
// Based on NVIDIA Falcor's IsotropicGGX.slang

import material.bsdf.math_constants;
import material.bsdf.math_helpers;

// Evaluates the GGX (Trowbridge-Reitz) normal distribution function (D).
// See Walter et al. "Microfacet Models for Refraction through Rough Surfaces"
//
// @param alpha GGX width parameter (should be clamped to small epsilon beforehand).
// @param cosTheta Dot product between shading normal and half vector.
// @return D(h)
float evalNdfGGX(float alpha, float cosTheta)
{
    float a2 = alpha * alpha;
    float d = ((cosTheta * a2 - cosTheta) * cosTheta + 1.0f);
    return a2 / (d * d * M_PI);
}

// Evaluates the PDF for sampling the GGX NDF using Walter et al.'s method.
//
// @param alpha GGX width parameter.
// @param cosTheta Dot product between shading normal and half vector.
// @return D(h) * cosTheta
float evalPdfGGX_NDF(float alpha, float cosTheta)
{
    return evalNdfGGX(alpha, cosTheta) * cosTheta;
}

// Samples the GGX NDF using Walter et al.'s method.
// The sampled half vector may lie in the negative hemisphere (should be discarded).
//
// @param alpha GGX width parameter.
// @param u Uniform random number (2D).
// @param pdf Output: sampling probability.
// @return Sampled half vector in local space.
float3 sampleGGX_NDF(float alpha, float2 u, out float pdf)
{
    float alphaSqr = alpha * alpha;
    float phi = u.y * (2.0f * M_PI);
    float tanThetaSqr = alphaSqr * u.x / (1.0f - u.x);
    float cosTheta = 1.0f / sqrt(1.0f + tanThetaSqr);
    float r = sqrt(max(1.0f - cosTheta * cosTheta, 0.0f));

    pdf = evalPdfGGX_NDF(alpha, cosTheta);
    return float3(cos(phi) * r, sin(phi) * r, cosTheta);
}

// Evaluates the Smith masking function G1 for GGX.
// See Eq 34 in Walter et al.
//
// @param alphaSqr Squared GGX width parameter.
// @param cosTheta Dot product between shading normal and evaluated direction.
// @return G1(cosTheta)
float evalG1GGX(float alphaSqr, float cosTheta)
{
    if (cosTheta <= 0.0f) return 0.0f;
    float cosThetaSqr = cosTheta * cosTheta;
    float tanThetaSqr = max(1.0f - cosThetaSqr, 0.0f) / cosThetaSqr;
    return 2.0f / (1.0f + sqrt(1.0f + alphaSqr * tanThetaSqr));
}

// Evaluates the Smith lambda function for GGX.
// See Eq 72 in Heitz "Understanding the Masking-Shadowing Function"
//
// @param alphaSqr Squared GGX width parameter.
// @param cosTheta Dot product between shading normal and evaluated direction.
// @return Lambda(cosTheta)
float evalLambdaGGX(float alphaSqr, float cosTheta)
{
    if (cosTheta <= 0.0f) return 0.0f;
    float cosThetaSqr = cosTheta * cosTheta;
    float tanThetaSqr = max(1.0f - cosThetaSqr, 0.0f) / cosThetaSqr;
    return 0.5f * (-1.0f + sqrt(1.0f + alphaSqr * tanThetaSqr));
}

// Evaluates the separable form of the masking-shadowing function (G).
// See Eq 98 in Heitz.
//
// @param alpha GGX width parameter.
// @param cosThetaI Dot product between shading normal and incident direction.
// @param cosThetaO Dot product between shading normal and outgoing direction.
// @return G(cosThetaI, cosThetaO)
float evalMaskingSmithGGXSeparable(float alpha, float cosThetaI, float cosThetaO)
{
    float alphaSqr = alpha * alpha;
    float lambdaI = evalLambdaGGX(alphaSqr, cosThetaI);
    float lambdaO = evalLambdaGGX(alphaSqr, cosThetaO);
    return 1.0f / ((1.0f + lambdaI) * (1.0f + lambdaO));
}

// Evaluates the height-correlated form of the masking-shadowing function.
// See Eq 99 in Heitz. More accurate than separable form.
//
// @param alpha GGX width parameter.
// @param cosThetaI Dot product between shading normal and incident direction.
// @param cosThetaO Dot product between shading normal and outgoing direction.
// @return G(cosThetaI, cosThetaO)
float evalMaskingSmithGGXCorrelated(float alpha, float cosThetaI, float cosThetaO)
{
    float alphaSqr = alpha * alpha;
    float lambdaI = evalLambdaGGX(alphaSqr, cosThetaI);
    float lambdaO = evalLambdaGGX(alphaSqr, cosThetaO);
    return 1.0f / (1.0f + lambdaI + lambdaO);
}

// Evaluates the PDF for sampling the GGX VNDF (visible normal distribution).
// See Heitz "Sampling the GGX Distribution of Visible Normals"
//
// @param alpha GGX width parameter.
// @param wi Incident direction in local space.
// @param h Half vector in local space.
// @return D_V(h) = G1(wi) * D(h) * max(0, dot(wi, h)) / wi.z
float evalPdfGGX_VNDF(float alpha, float3 wi, float3 h)
{
    float G1 = evalG1GGX(alpha * alpha, wi.z);
    float D = evalNdfGGX(alpha, h.z);
    return G1 * D * max(0.0f, dot(wi, h)) / wi.z;
}

// Helper for VNDF sampling using spherical caps method.
// "Sampling Visible GGX Normals with Spherical Caps" by Dupuy and Benyoub [2023]
float3 sampleVndf_Hemisphere(float2 u, float3 Vh)
{
    float phi = 2.0f * M_PI * u.x;
    float z = (1.0f - u.y) * (1.0f + Vh.z) - Vh.z;
    float sinTheta = sqrt(saturate(1.0f - z * z));
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    float3 c = float3(x, y, z);
    float3 h = c + Vh;
    return h;
}

// Samples the GGX distribution using VNDF (visible normal distribution).
// Yields significant variance reduction compared to NDF sampling.
//
// @param alpha Isotropic GGX width parameter.
// @param wi Incident direction in local space.
// @param u Uniform random number (2D).
// @param pdf Output: sampling probability.
// @return Sampled half vector in local space.
float3 sampleGGX_VNDF(float alpha, float3 wi, float2 u, out float pdf)
{
    // Transform view vector to hemisphere configuration
    float3 Vh = normalize(float3(alpha * wi.x, alpha * wi.y, wi.z));

    // Sample visible hemisphere
    float3 Nh = sampleVndf_Hemisphere(u, Vh);

    // Transform back to ellipsoid configuration
    float3 h = normalize(float3(alpha * Nh.x, alpha * Nh.y, Nh.z));

    pdf = evalPdfGGX_VNDF(alpha, wi, h);
    return h;
}

// Approximate pre-integrated specular BRDF for split-sum approximation.
// See Ray Tracing Gems, Chapter 32
//
// @param specularReflectance Reflectance at normal incidence.
// @param alpha GGX width parameter.
// @param cosTheta Dot product between shading normal and view direction.
// @return Approximate integral of specular BRDF.
float3 approxSpecularIntegralGGX(float3 specularReflectance, float alpha, float cosTheta)
{
    cosTheta = abs(cosTheta);

    float4 X = float4(1.0f, cosTheta, cosTheta * cosTheta, cosTheta * cosTheta * cosTheta);
    float4 Y = float4(1.0f, alpha, alpha * alpha, alpha * alpha * alpha);

    // Coefficients for correlated G term
    float2x2 M1 = float2x2(
        0.995367f, -1.38839f,
        -0.24751f, 1.97442f
    );

    float3x3 M2 = float3x3(
        1.0f, 2.68132f, 52.366f,
        16.0932f, -3.98452f, 59.3013f,
        -5.18731f, 255.259f, 2544.07f
    );

    float2x2 M3 = float2x2(
        -0.0564526f, 3.82901f,
        16.91f, -11.0303f
    );

    float3x3 M4 = float3x3(
        1.0f, 4.11118f, -1.37886f,
        19.3254f, -28.9947f, 16.9514f,
        0.545386f, 96.0994f, -79.4492f
    );

    float bias = dot(mul(M1, X.xy), Y.xy) / dot(mul(M2, float3(X.x, X.y, X.w)), float3(Y.x, Y.y, Y.w));
    float scale = dot(mul(M3, X.xy), Y.xy) / dot(mul(M4, float3(X.x, X.z, X.w)), float3(Y.x, Y.y, Y.w));

    // Hack for specular reflectance of 0
    float specularReflectanceLuma = dot(specularReflectance, float3(1.0f / 3.0f));
    bias *= saturate(specularReflectanceLuma * 50.0f);

    return specularReflectance * max(0.0f, scale) + max(0.0f, bias);
}
