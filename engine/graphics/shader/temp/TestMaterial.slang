import material.IMaterial;

struct MaterialImpl : IMaterial
{
    float4 uColor;
    // Texture2D uTex;
    // SamplerState uSampler;
    // uint2 uTexIndex;
    // uint2 uSamplerIndex;

    func evalSurface(float2 uv, float3 worldPos) -> SurfaceData
    {
        // Texture2D.Handle uTex = (Texture2D.Handle)uTexIndex;
        // SamplerState.Handle uSampler = (SamplerState.Handle)uSamplerIndex;
        SurfaceData s;
        // float4 t = uTex.Sample(uSampler, uv);
        // s.baseColor = uColor.rgb * t.rgb;
        s.baseColor = uColor.rgb;
        // s.baseColor = t.xyz;
        s.normal = float3(0,1,0);
        s.roughness = 0.5;
        s.metallic = 0.0;
        s.emissive = 0.0;
        s.opacity = 1.0;
        return s;
    }
};
