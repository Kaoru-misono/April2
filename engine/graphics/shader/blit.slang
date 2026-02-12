
#include "full_screen.slang"

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

uniform float4 uvTransform; // xy: offset, zw: scale

[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    FullScreenVertex fs = makeFullScreenTriangle(vertexID);

    VertexOutput output;
    output.position = fs.position;
    output.uv = fs.uv * uvTransform.zw + uvTransform.xy;
    return output;
}

uniform Texture2D sourceTexture;
uniform SamplerState sourceSampler;

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return sourceTexture.Sample(sourceSampler, input.uv);
}
