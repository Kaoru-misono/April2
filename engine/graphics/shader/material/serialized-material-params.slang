// Falcor-style serialized material parameter payload scaffolding.

struct SerializedMaterialParam
{
    uint nameHash;
    uint type;
    uint dataOffset;
    uint dataSize;
};

struct SerializedMaterialParams
{
    uint paramCount;
    StructuredBuffer<SerializedMaterialParam> params;
    ByteAddressBuffer data;
};
