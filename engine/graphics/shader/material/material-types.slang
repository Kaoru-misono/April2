// Material type definitions for the April Engine material system.
// Based on NVIDIA Falcor's material system architecture.

// @export-cpp
enum class LobeType : uint
{
    None                    = 0x00,

    DiffuseReflection       = 0x01,
    SpecularReflection      = 0x02,
    DeltaReflection         = 0x04,

    DiffuseTransmission     = 0x10,
    SpecularTransmission    = 0x20,
    DeltaTransmission       = 0x40,

    Diffuse                 = 0x11,
    Specular                = 0x22,
    Delta                   = 0x44,
    NonDelta                = 0x33,

    Reflection              = 0x0f,
    Transmission            = 0xf0,

    NonDeltaReflection      = 0x03,
    NonDeltaTransmission    = 0x30,

    All                     = 0xff,
};

// @export-cpp
enum class AlphaMode : uint
{
    Opaque = 0,
    Mask   = 1,
    Blend  = 2,
};

// @export-cpp
enum class MaterialType : uint
{
    Unknown  = 0,
    Standard = 1,
};

// @export-cpp
enum class MaterialFlags : uint
{
    None              = 0x00,
    DoubleSided       = 0x01,
    AlphaTested       = 0x02,
    Emissive          = 0x04,
    ThinSurface       = 0x08,
    HasTextures       = 0x10,
    SpecularWorkflow  = 0x20,
};

// @export-cpp
enum class TextureSlot : uint
{
    BaseColor         = 0,
    MetallicRoughness = 1,
    Normal            = 2,
    Occlusion         = 3,
    Emissive          = 4,
    Count             = 5,
};
