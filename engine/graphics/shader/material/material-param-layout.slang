// Falcor-style material parameter layout scaffolding.

enum MaterialParamType : uint
{
    Unknown = 0,
    Float = 1,
    Float2 = 2,
    Float3 = 3,
    Float4 = 4,
    Uint = 5,
    Texture2D = 6,
    Sampler = 7,
};

struct MaterialParamLayoutEntry
{
    uint nameHash;
    uint offset;
    uint size;
    uint type;
};

struct MaterialParamLayout
{
    uint entryCount;
    StructuredBuffer<MaterialParamLayoutEntry> entries;
};
