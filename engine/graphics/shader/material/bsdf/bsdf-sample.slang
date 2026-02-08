// BSDF sample structures and context.

import material.material_types;

// Albedo contributions split into reflection and transmission.
struct AlbedoContributions
{
    float3 reflection;      // Energy reflected into upper hemisphere
    float3 absorptionR;     // Energy absorbed in upper hemisphere
    float3 transmission;    // Energy transmitted into lower hemisphere
    float3 absorptionT;     // Energy absorbed in lower hemisphere

    __init(float3 r, float3 ar, float3 t, float3 at)
    {
        reflection = r;
        absorptionR = ar;
        transmission = t;
        absorptionT = at;
    }

    __init()
    {
        reflection = float3(0.0f);
        absorptionR = float3(0.0f);
        transmission = float3(0.0f);
        absorptionT = float3(0.0f);
    }
};

AlbedoContributions operator+(AlbedoContributions a, AlbedoContributions b)
{
    return AlbedoContributions(
        a.reflection + b.reflection,
        a.absorptionR + b.absorptionR,
        a.transmission + b.transmission,
        a.absorptionT + b.absorptionT
    );
}

AlbedoContributions operator*(float f, AlbedoContributions b)
{
    return AlbedoContributions(
        f * b.reflection,
        f * b.absorptionR,
        f * b.transmission,
        f * b.absorptionT
    );
}

// Context for BSDF evaluation containing IOR information.
struct BSDFContext
{
    float iorI;     // IOR from incidence medium
    float iorT;     // IOR from transmission medium
    bool inited;    // Flag to indicate if the struct was initialized

    __init(float iorI_, float iorT_)
    {
        iorI = iorI_;
        iorT = iorT_;
        inited = true;
    }

    __init()
    {
        iorI = 1.0f;
        iorT = 1.0f;
        inited = false;
    }
};

// Information about the roughness of a BSDF.
struct RoughnessInformation
{
    float2 roughnessBSDFNotation;   // Roughness in BSDF notation
    float3 scratch;                  // Additional data

    __init()
    {
        roughnessBSDFNotation = float2(0.0f);
        scratch = float3(0.0f);
    }
};

// BSDF properties for denoising and other purposes.
struct BSDFProperties
{
    float3 albedo;              // Overall albedo
    float roughness;            // Linear roughness
    float3 emission;            // Emitted radiance
    uint flags;                 // Property flags

    __init()
    {
        albedo = float3(0.0f);
        roughness = 0.0f;
        emission = float3(0.0f);
        flags = 0;
    }
};
