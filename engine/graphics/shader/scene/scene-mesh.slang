// Scene mesh shader with material support.

import material.material_types;
import material.material_data;
import material.material_system;
import material.material_factory;
import material.i_material;
import material.i_material_instance;
import material.shading_data;
import material.shading_utils;
import material.texture_sampler;
import material.material_instance_hints;

// Material system defines must be injected from host.
#if !defined(MATERIAL_SYSTEM_TEXTURE_DESC_COUNT) || !defined(MATERIAL_SYSTEM_SAMPLER_DESC_COUNT) || !defined(MATERIAL_SYSTEM_BUFFER_DESC_COUNT)
#error "MaterialSystem defines are not set!"
#endif

struct VSIn
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texCoord : TEXCOORD;
};

struct VSOut
{
    float4 pos      : SV_Position;
    float3 worldPos : WORLD_POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct PerFrame
{
    float4x4 viewProj;
    float3 cameraPos;
    float _pad0;
};

struct PerInstance
{
    float4x4 model;
    uint materialIndex;
    uint3 _pad;
};

ParameterBlock<PerFrame> perFrame;
ParameterBlock<PerInstance> perInstance;
ParameterBlock<MaterialSystem> materials;

struct SceneDummySampleGenerator : ISampleGenerator
{
    float next1D() { return 0.5f; }
    float2 next2D() { return float2(0.5f, 0.5f); }
};

[shader("vertex")]
VSOut vsMain(VSIn input)
{
    VSOut output;
    float4 worldPos = mul(perInstance.model, float4(input.position, 1.0));
    output.pos = mul(perFrame.viewProj, worldPos);
    output.worldPos = worldPos.xyz;
    output.normal = normalize(mul((float3x3)perInstance.model, input.normal));
    output.texCoord = input.texCoord;
    return output;
}

[shader("fragment")]
float4 psMain(VSOut input, bool isFrontFace : SV_IsFrontFace) : SV_Target
{
    MaterialHeader mtl = materials.getMaterialHeader(perInstance.materialIndex);

    float3 N = normalize(input.normal);
    float3 V = normalize(perFrame.cameraPos - input.worldPos);
    float3 T = makeSafeTangent(N);
    ShadingData sd = prepareShadingData(
        input.worldPos,
        V,
        N,
        T,
        input.texCoord,
        N,
        isFrontFace,
        perInstance.materialIndex,
        mtl,
        1.0f
    );

    ImplicitLodTextureSampler lod;
    IMaterialInstance materialInstance = materials.getMaterialInstance(sd, lod, (uint)MaterialInstanceHints::None);
    BSDFProperties props = materialInstance.getProperties(sd);
    float3 baseAlbedo = props.albedo;
    float roughness = props.roughness;
    float3 emissive = props.emission;

    // Basic PBR lighting
    float3 L = normalize(float3(1.0, 1.0, 1.0));  // Fixed directional light
    SceneDummySampleGenerator sg;
    float3 bsdf = materialInstance.eval(sd, L, sg);
    float phase = materials.evalPhaseFunction(perInstance.materialIndex, dot(-V, L));

    // Ambient
    float3 ambient = baseAlbedo * 0.03;

    if (props.isTransmissive)
    {
        bsdf += phase * 0.02;
    }

    // Final color
    float3 finalColor = ambient + bsdf + emissive;

    return float4(finalColor, 1.0f);
}
