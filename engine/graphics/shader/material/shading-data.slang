// Shading data structure containing geometry information at a shading point.

// Geometry information at a shading point.
struct ShadingData
{
    float3 posW;            // World-space position
    float3 V;               // View direction (world-space, pointing to camera)
    float3 N;               // Shading normal (world-space)
    float3 T;               // Shading tangent (world-space)
    float3 B;               // Shading bitangent (world-space)
    float2 uv;              // Texture coordinates
    float3 faceN;           // Face normal (world-space, geometric normal)
    bool frontFacing;       // True if viewing the front face
    float NdotV;            // Dot product of N and V (clamped)

    // Transform a direction from world space to local shading space.
    float3 toLocal(float3 wWorld)
    {
        return float3(dot(wWorld, T), dot(wWorld, B), dot(wWorld, N));
    }

    // Transform a direction from local shading space to world space.
    float3 toWorld(float3 wLocal)
    {
        return wLocal.x * T + wLocal.y * B + wLocal.z * N;
    }
};

// Create shading data from geometric information.
ShadingData prepareShadingData(
    float3 posW,
    float3 V,
    float3 N,
    float3 T,
    float2 uv,
    float3 faceN,
    bool frontFacing
)
{
    ShadingData sd;
    sd.posW = posW;
    sd.V = V;
    sd.N = N;
    sd.T = T;
    sd.B = cross(N, T);
    sd.uv = uv;
    sd.faceN = faceN;
    sd.frontFacing = frontFacing;
    sd.NdotV = max(dot(N, V), 0.0f);
    return sd;
}
