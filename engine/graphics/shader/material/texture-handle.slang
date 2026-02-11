// TextureHandle aligned with Falcor Scene/Material/TextureHandle.slang.

// @export-cpp
struct TextureHandle
{
    uint packedData = 0;

    enum class Mode : uint
    {
        Uniform = 0,
        Texture = 1,
    };

    static const uint kTextureIDBits = 29;
    static const uint kModeBits = 2;
    static const uint kUdimEnabledBits = 1;

    static const uint kModeOffset = kTextureIDBits;
    static const uint kUdimEnabledOffset = kModeOffset + kModeBits;

    [mutating]
    void setMode(Mode mode)
    {
        uint modeMask = ((1u << kModeBits) - 1u) << kModeOffset;
        packedData = (packedData & ~modeMask) | ((uint(mode) << kModeOffset) & modeMask);
    }

    Mode getMode()
    {
        return Mode((packedData >> kModeOffset) & ((1u << kModeBits) - 1u));
    }

    [mutating]
    void setTextureID(uint texID)
    {
        setMode(Mode::Texture);
        uint idMask = (1u << kTextureIDBits) - 1u;
        packedData = (packedData & ~idMask) | (texID & idMask);
    }

    uint getTextureID()
    {
        return packedData & ((1u << kTextureIDBits) - 1u);
    }

    [mutating]
    void setUdimEnabled(bool udimEnabled)
    {
        uint udimMask = ((1u << kUdimEnabledBits) - 1u) << kUdimEnabledOffset;
        packedData = (packedData & ~udimMask) | ((udimEnabled ? 1u : 0u) << kUdimEnabledOffset);
    }

    bool getUdimEnabled()
    {
        return (packedData & (1u << kUdimEnabledOffset)) != 0;
    }
};

// @export-cpp
struct TextureInfo
{
    uint width;
    uint height;
    uint depth;
    uint mipLevels;
};
