struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    // Generate full-screen triangle
    // ID 0: (-1, -1) -> UV (0, 0)
    // ID 1: (-1,  3) -> UV (0, 2)
    // ID 2: ( 3, -1) -> UV (2, 0)
    // Clip space: [-1, 1] for X, Y. Y is usually fliped in some APIs but Slang/Vulkan standard is Y down?
    // Let's use the standard "large triangle" trick.
    
    // float2 texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    // output.position = float4(texCoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    // output.uv = texCoord;

    // Alternative clear way:
    float2 grid = float2(uint2((vertexID << 1) & 2, vertexID & 2));
    // grid is (0,0), (2,0), (0,2) ... wait
    // vertexID 0: (0<<1)&2 = 0, 0&2 = 0 -> (0,0)
    // vertexID 1: (1<<1)&2 = 2, 1&2 = 0 -> (2,0)
    // vertexID 2: (2<<1)&2 = 4&2=0, 2&2=2 -> (0,2)
    // So we have (0,0), (2,0), (0,2).
    
    // Map to Clip Space [-1, 1]
    // (0,0) -> (-1, 1)  (Top-Left)
    // (2,0) -> ( 3, 1)  (Far Right)
    // (0,2) -> (-1, -3) (Far Bottom)
    
    // Note: Slang/Vulkan Y is down. (-1, -1) is top-left? No, (-1,-1) is top-left in OpenGL.
    // In Vulkan/D3D12 (which Slang targets often):
    // (-1, -1) is Top-Left? No.
    // D3D: (-1, 1) Top-Left, (1, -1) Bottom-Right. Z [0, 1].
    // Vulkan: (-1, -1) Top-Left. Z [0, 1].
    
    // ImGui example used: output.position.y *= -1.0; implies they might be flipping.
    
    // Let's stick to standard full screen triangle and if it's upside down we fix it.
    // UV (0,0) is usually Top-Left.
    
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.uv = uv;
    output.position = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, 0.0f, 1.0f);
    
    return output;
}

uniform Texture2D sourceTexture;
uniform SamplerState sourceSampler;

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return sourceTexture.Sample(sourceSampler, input.uv);
}
