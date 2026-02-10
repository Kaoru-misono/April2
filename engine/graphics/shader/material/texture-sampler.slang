// Texture sampler abstractions for material pattern generation.

interface ITextureSampler
{
    float4 sampleTexture(Texture2D texture, SamplerState sampler, float2 uv);
};

struct ImplicitLodTextureSampler : ITextureSampler
{
    float4 sampleTexture(Texture2D texture, SamplerState sampler, float2 uv)
    {
        return texture.Sample(sampler, uv);
    }
};

struct ExplicitLodTextureSampler : ITextureSampler
{
    float lod;

    __init(float lod_)
    {
        lod = lod_;
    }

    float4 sampleTexture(Texture2D texture, SamplerState sampler, float2 uv)
    {
        return texture.SampleLevel(sampler, uv, lod);
    }
};
