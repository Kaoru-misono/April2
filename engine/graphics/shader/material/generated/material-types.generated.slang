// AUTO-GENERATED from shader/shared/material/material-types.shared.slang - DO NOT EDIT
// Generated shader-side declarations from shared schema.

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

enum class AlphaMode : uint
{

        Opaque = 0,
        Mask   = 1,

};

enum class MaterialType : uint
{

        Unknown  = 0,
        Standard = 1,

};

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

enum class TextureSlot : uint
{

        BaseColor    = 0,
        Specular     = 1,
        Emissive     = 2,
        Normal       = 3,
        Transmission = 4,
        Displacement = 5,
        Count        = 6,

};

enum class ShadingModel : uint
{

        MetalRough = 0,
        SpecGloss = 1,
        Count = 2,

};

enum class NormalMapType : uint
{

        None = 0,
        RG = 1,
        RGB = 2,
        Count = 3,

};
