struct FullScreenVertex
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

inline FullScreenVertex makeFullScreenTriangle(uint vertexID)
{
    FullScreenVertex output;
    float2 uvRaw = float2((vertexID << 1) & 2, vertexID & 2);
    output.uv = uvRaw;
    output.position = float4(uvRaw * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}
