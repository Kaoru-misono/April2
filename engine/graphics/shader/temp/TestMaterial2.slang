import material.IMaterial;

// 约定：主实现类必须命名为 "MaterialImpl"
struct MaterialImpl2 : IMaterial
{
    float4 uColor;
    Texture2D uTex;
    SamplerState uSampler;

    func evalSurface(float2 uv, float3 worldPos) -> SurfaceData
    {
        SurfaceData s;
        float4 t = uTex.Sample(uSampler, uv);
        // s.baseColor = uColor.rgb * t.rgb;
        s.baseColor = float3(1.0, 0.0, 0.0);
        s.normal = float3(0,1,0);
        s.roughness = 0.5;
        s.metallic = 0.0;
        s.emissive = 0.0;
        s.opacity = 1.0;
        return s;
    }
};
