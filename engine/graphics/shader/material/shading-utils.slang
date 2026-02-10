// Shared shading helper utilities.

float3 makeSafeTangent(float3 n)
{
    float3 tangentRef = abs(n.z) < 0.99f ? float3(0.0f, 0.0f, 1.0f) : float3(0.0f, 1.0f, 0.0f);
    return normalize(cross(tangentRef, n));
}
