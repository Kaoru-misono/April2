// Scene mesh shader with material support.

import material.material_types;
import material.material_data;
import material.material_system;
import material.material_factory;
import material.i_material;
import material.shading_data;

// Descriptor table capacities - host-provided via defines, with defaults.
#ifndef MATERIAL_TEXTURE_TABLE_SIZE
#define MATERIAL_TEXTURE_TABLE_SIZE 128
#endif

#ifndef MATERIAL_SAMPLER_TABLE_SIZE
#define MATERIAL_SAMPLER_TABLE_SIZE 8
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
StructuredBuffer<StandardMaterialData> materials;

Texture2D materialTextures[kMaterialTextureTableSize];
SamplerState materialSamplers[kMaterialSamplerTableSize];

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
    MaterialSystemContext materialSystem;
    materialSystem.materials = materials;
    materialSystem.materialTextures = materialTextures;
    materialSystem.materialSamplers = materialSamplers;

    StandardMaterialData mtl = getMaterial(materialSystem, perInstance.materialIndex);
    MaterialFactory materialFactory = MaterialFactory(materialSystem);

    float3 N = normalize(input.normal);
    float3 V = normalize(perFrame.cameraPos - input.worldPos);
    float3 tangentRef = abs(N.z) < 0.99f ? float3(0.0f, 0.0f, 1.0f) : float3(0.0f, 1.0f, 0.0f);
    float3 T = normalize(cross(tangentRef, N));
    ShadingData sd = prepareShadingData(
        input.worldPos,
        V,
        N,
        T,
        input.texCoord,
        N,
        isFrontFace,
        perInstance.materialIndex,
        mtl.header,
        1.0f
    );

    IMaterialInstance materialInstance = getMaterialInstance(materialFactory, perInstance.materialIndex, sd);
    float3 baseAlbedo = materialInstance.getAlbedo(sd);
    float roughness = materialInstance.getRoughness(sd);
    float3 emissive = materialInstance.getEmission(sd);

    // Basic PBR lighting
    float3 L = normalize(float3(1.0, 1.0, 1.0));  // Fixed directional light
    float3 bsdf = materialInstance.eval(sd, L);

    // Ambient
    float3 ambient = baseAlbedo * 0.03;

    // Final color
    float3 finalColor = ambient + bsdf + emissive;

    return float4(finalColor, 1.0f);
}
