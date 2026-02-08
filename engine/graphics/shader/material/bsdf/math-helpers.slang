// Math helper functions for BSDF calculations.

import material.bsdf.math_constants;

// Luminance of an RGB color using Rec. 709 coefficients.
float luminance(float3 rgb)
{
    return dot(rgb, float3(0.2126f, 0.7152f, 0.0722f));
}

// Saturate (clamp to [0, 1])
float saturate(float x)
{
    return clamp(x, 0.0f, 1.0f);
}

float3 saturate(float3 x)
{
    return clamp(x, float3(0.0f), float3(1.0f));
}

// Sign function that returns 1 for zero input (never returns 0)
float signNotZero(float x)
{
    return x >= 0.0f ? 1.0f : -1.0f;
}

// Safe sqrt that clamps negative values to zero
float safeSqrt(float x)
{
    return sqrt(max(x, 0.0f));
}

// Cosine-weighted hemisphere sampling using concentric disk mapping
// Returns the sampled direction in the local frame where z is up
float3 sample_cosine_hemisphere_concentric(float2 u, out float pdf)
{
    // Map uniform random numbers to [-1, 1]
    float2 uOffset = 2.0f * u - float2(1.0f, 1.0f);

    // Handle degeneracy at the origin
    if (uOffset.x == 0.0f && uOffset.y == 0.0f)
    {
        pdf = M_1_PI;
        return float3(0.0f, 0.0f, 1.0f);
    }

    // Apply concentric mapping to point
    float theta, r;
    if (abs(uOffset.x) > abs(uOffset.y))
    {
        r = uOffset.x;
        theta = M_PI_4 * (uOffset.y / uOffset.x);
    }
    else
    {
        r = uOffset.y;
        theta = M_PI_2 - M_PI_4 * (uOffset.x / uOffset.y);
    }

    float2 diskPoint = r * float2(cos(theta), sin(theta));

    // Project up to hemisphere
    float z = safeSqrt(1.0f - dot(diskPoint, diskPoint));
    pdf = z * M_1_PI;
    return float3(diskPoint.x, diskPoint.y, z);
}
