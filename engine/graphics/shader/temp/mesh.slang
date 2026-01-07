// assets/shader/mesh.slang

struct Uniforms
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float4 color;
    bool useTexture;
};

ConstantBuffer<Uniforms> ubo;
uniform Texture2D baseColor;
uniform SamplerState sampler;

struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 uv       : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
    float4 color    : COLOR;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    float4 worldPos = mul(ubo.model, float4(input.position, 1.0));
    float4 viewPos  = mul(ubo.view, worldPos);
    output.position = mul(ubo.proj, viewPos);

    output.normal = mul((float3x3)ubo.model, input.normal);
    output.uv = input.uv;
    output.color = ubo.color;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float3 lightDir = normalize(float3(1.0, 1.0, 1.0));
    float diff = max(dot(normalize(input.normal), lightDir), 0.1);

    float3 color = ubo.useTexture ? baseColor.Sample(sampler, input.uv).xyz : input.color.xyz * diff;
    return float4(1.0, 0.0, 0.0, 1.0);
}
