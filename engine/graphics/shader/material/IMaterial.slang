struct SurfaceData
{
    float3 baseColor;
    float3 normal;
    float3 emissive;
    float roughness;
    float metallic;
    float ao;
    float opacity;
};

interface IMaterial
{
    func evalSurface(float2 uv, float3 worldPos) -> SurfaceData;
};
