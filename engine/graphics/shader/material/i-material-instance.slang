// IMaterialInstance interface and shared structs.

import material.shading_data;
import material.material_types;
import material.bsdf.i_bsdf;
import material.bsdf.bsdf_sample;
import material.bsdf.sample_generator;
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

#ifndef FALCOR_MATERIAL_INSTANCE_SIZE
#error Missing FALCOR_MATERIAL_INSTANCE_SIZE
#endif

[anyValueSize(FALCOR_MATERIAL_INSTANCE_SIZE)]
interface IMaterialInstance
{
    float3 eval<S : ISampleGenerator>(ShadingData sd, float3 wo, inout S sg);
    float3 evalAD<S : ISampleGenerator>(DiffMaterialData diffData, ShadingData sd, float3 wo, inout S sg);

    bool sample<S : ISampleGenerator>(ShadingData sd, inout S sg, out BSDFSample result, bool useImportanceSampling = true);

    float evalPdf(ShadingData sd, float3 wo, bool useImportanceSampling = true);

    BSDFProperties getProperties(ShadingData sd);
    uint getLobeTypes(ShadingData sd);

    bool hasVolumeProperties();
    VolumeProperties getVolumeProperties();
};

[open]
struct MaterialInstanceBase
{
    bool hasVolumeProperties()
    {
        return false;
    }

    VolumeProperties getVolumeProperties()
    {
        VolumeProperties result;
        result.sigmaA = 0.0f;
        result.sigmaS = 0.0f;
        result.g = 0.0f;
        return result;
    }
};
