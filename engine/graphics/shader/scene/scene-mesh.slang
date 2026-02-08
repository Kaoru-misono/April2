// Scene mesh shader with material support.

import material.material_types;
import material.material_data;

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

// Material textures
Texture2D baseColorTexture;
Texture2D metallicRoughnessTexture;
Texture2D normalTexture;
Texture2D occlusionTexture;
Texture2D emissiveTexture;
SamplerState materialSampler;

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
float4 psMain(VSOut input) : SV_Target
{
    StandardMaterialData mtl = materials[perInstance.materialIndex];

    // Sample base color
    float4 baseColor = mtl.baseColor;
    if ((mtl.header.flags & (uint)MaterialFlags.HasTextures) != 0)
    {
        float4 texColor = baseColorTexture.Sample(materialSampler, input.texCoord);
        baseColor *= texColor;
    }

    // Alpha testing
    if (mtl.header.alphaMode == (uint)AlphaMode.Mask)
    {
        if (baseColor.a < mtl.alphaCutoff)
            discard;
    }

    // Sample metallic/roughness
    float metallic = mtl.metallic;
    float roughness = mtl.roughness;
    if ((mtl.header.flags & (uint)MaterialFlags.HasTextures) != 0)
    {
        float4 mrSample = metallicRoughnessTexture.Sample(materialSampler, input.texCoord);
        // glTF convention: roughness in G, metallic in B
        roughness *= mrSample.g;
        metallic *= mrSample.b;
    }

    // Sample emissive
    float3 emissive = mtl.emissive;
    if ((mtl.header.flags & (uint)MaterialFlags.Emissive) != 0)
    {
        float3 emissiveSample = emissiveTexture.Sample(materialSampler, input.texCoord).rgb;
        emissive *= emissiveSample;
    }

    // Basic PBR lighting
    float3 N = normalize(input.normal);
    float3 V = normalize(perFrame.cameraPos - input.worldPos);
    float3 L = normalize(float3(1.0, 1.0, 1.0));  // Fixed directional light
    float3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    // Simple approximation of PBR
    // Diffuse contribution (non-metallic)
    float3 diffuseColor = baseColor.rgb * (1.0 - metallic);
    float3 diffuse = diffuseColor * NdotL;

    // Specular contribution
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    float D = alpha2 / (3.14159265 * denom * denom);

    // Fresnel (Schlick approximation)
    float3 F0 = lerp(float3(0.04), baseColor.rgb, metallic);
    float3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);

    float3 specular = D * F * NdotL * 0.25;

    // Ambient
    float3 ambient = baseColor.rgb * 0.03;

    // Final color
    float3 finalColor = ambient + diffuse + specular + emissive;

    return float4(finalColor, baseColor.a);
}
