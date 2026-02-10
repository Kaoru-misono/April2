// IMaterialInstance interface and shared structs.

import material.shading_data;
import material.material_types;
import material.bsdf.i_bsdf;
import material.bsdf.bsdf_sample;
import material.material_instance_hints;

struct DiffMaterialData
{
    float4 payload;
};

struct VolumeProperties
{
    float3 sigmaA;
    float3 sigmaS;
    float g;
};

struct BSDFSample
{
    float3 wo;
    float pdf;
    float3 weight;
    uint lobeType;
};

[anyValueSize(128)]
interface IMaterialInstance
{
    MaterialType getType();

    IBSDF getBSDF(ShadingData sd);

    float3 eval(ShadingData sd, float3 wo);
    float3 evalAD(DiffMaterialData diffData, ShadingData sd, float3 wo);

    bool sample(ShadingData sd, float3 u, out float3 wo, out float pdf, out float3 weight, out uint lobeType);

    float evalPdf(ShadingData sd, float3 wo);

    BSDFProperties getProperties(ShadingData sd);
    uint getLobeTypes(ShadingData sd);

    bool hasVolumeProperties();
    VolumeProperties getVolumeProperties();

    float3 getEmission(ShadingData sd);
    float3 getAlbedo(ShadingData sd);
    float getRoughness(ShadingData sd);
};
