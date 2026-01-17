struct ImGuiUniforms
{
    float2 scale;
    float2 translate;
};

ConstantBuffer<ImGuiUniforms> ubo;

struct VertexInput
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR0;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR0;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = float4(input.position.xy * ubo.scale + ubo.translate, 0.0, 1.0);
    output.position.y *= -1.0;
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

uniform Texture2D fontTexture;
uniform SamplerState fontSampler;

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return input.color * fontTexture.Sample(fontSampler, input.uv);
}
