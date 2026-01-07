import material.IMaterial;

struct UBO
{
    float4x4 view;
    float4x4 proj;
    float3 cameraPos;
    float4x4 model;
};
ParameterBlock<UBO> gScene : register(t0, space1);

// struct MaterialData
// {
//     Texture2D color;
//     SamplerState sampler;
//
//     float3 getBaseColor(uint2 uv)
//     {
//         return color.Sample(sampler, uv).xyz;
//     }
// }
// ParameterBlock<MaterialData> gMaterialData;

struct VsOut
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    float3 worldPos : POSITION0;
};

[shader("vertex")]
VsOut vertexMain(float3 position: POSITION, float3 normal: NORMAL, float4 tangent: TANGENT, float2 uv: TEXCOORD)
{
    VsOut o;
    float4 worldPos = mul(gScene.model, float4(position, 1.0));
    o.worldPos = worldPos.xyz;
    o.pos = mul(gScene.proj, mul(gScene.view, worldPos));
    o.uv = uv;
    return o;
}

[shader("fragment")]
float4 fragmentMain<M : IMaterial>(VsOut stageIn, uniform M material) : SV_Target
{
    SurfaceData s = material.evalSurface(stageIn.uv, stageIn.worldPos);

    float3 lightDir = normalize(float3(1, 1, 1));
    float NdotL = max(dot(s.normal, lightDir), 0.0);
    // float3 finalColor = s.baseColor * (NdotL + 0.1);
    float3 finalColor = s.baseColor;

    return float4(finalColor, s.opacity);
}
