// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include <core/tools/enum-flags.hpp>
#include <core/tools/enum.hpp>
#include <core/log/logger.hpp>
#include <core/error/assert.hpp>
#include <string>
#include <cstdint>
#include <vector>
#include <span>

namespace april::graphics
{
    /**
     * Flags for enumerating texture color channels.
     */
    enum class TextureChannelFlags : uint32_t
    {
        None = 0x0,
        Red = 0x1,
        Green = 0x2,
        Blue = 0x4,
        Alpha = 0x8,
        RGB = 0x7,
        RGBA = 0xf,
    };
    AP_ENUM_CLASS_OPERATORS(TextureChannelFlags);

    /**
     * Resource formats
     */
    enum class ResourceFormat : uint32_t
    {
        Unknown,
        R8Unorm,
        R8Snorm,
        R16Unorm,
        R16Snorm,
        RG8Unorm,
        RG8Snorm,
        RG16Unorm,
        RG16Snorm,
        RGB5A1Unorm,
        RGBA8Unorm,
        RGBA8Snorm,
        RGB10A2Unorm,
        RGB10A2Uint,
        RGBA16Unorm,
        RGBA16Snorm,
        RGBA8UnormSrgb,
        R16Float,
        RG16Float,
        RGBA16Float,
        R32Float,
        RG32Float,
        RGB32Float,
        RGBA32Float,
        R11G11B10Float,
        RGB9E5Float,
        R8Int,
        R8Uint,
        R16Int,
        R16Uint,
        R32Int,
        R32Uint,
        RG8Int,
        RG8Uint,
        RG16Int,
        RG16Uint,
        RG32Int,
        RG32Uint,
        RGB32Int,
        RGB32Uint,
        RGBA8Int,
        RGBA8Uint,
        RGBA16Int,
        RGBA16Uint,
        RGBA32Int,
        RGBA32Uint,

        BGRA4Unorm,
        BGRA8Unorm,
        BGRA8UnormSrgb,

        BGRX8Unorm,
        BGRX8UnormSrgb,
        R5G6B5Unorm,

        // Depth-stencil
        D32Float,
        D32FloatS8Uint,
        D16Unorm,

        // Compressed formats
        BC1Unorm, // DXT1
        BC1UnormSrgb,
        BC2Unorm, // DXT3
        BC2UnormSrgb,
        BC3Unorm, // DXT5
        BC3UnormSrgb,
        BC4Unorm, // RGTC Unsigned Red
        BC4Snorm, // RGTC Signed Red
        BC5Unorm, // RGTC Unsigned RG
        BC5Snorm, // RGTC Signed RG
        BC6HS16,
        BC6HU16,
        BC7Unorm,
        BC7UnormSrgb,

        Count
    };

    /**
     * Falcor format Type
     */
    enum class FormatType
    {
        Unknown,   ///< Unknown format Type
        Float,     ///< Floating-point formats
        Unorm,     ///< Unsigned normalized formats
        UnormSrgb, ///< Unsigned normalized SRGB formats
        Snorm,     ///< Signed normalized formats
        Uint,      ///< Unsigned integer formats
        Sint       ///< Signed integer formats
    };

    struct FormatDesc
    {
        ResourceFormat format{};
        std::string const name{};
        uint32_t bytesPerBlock{};
        uint32_t channelCount{};
        FormatType type{};
        bool isDepth{};
        bool isStencil{};
        bool isCompressed{};
        struct
        {
            uint32_t width;
            uint32_t height;
        } compressionRatio{};
        int numChannelBits[4]{};
    };

    // Extern declaration for the format description table
    extern FormatDesc const kFormatDesc[];

    /**
     * Get the number of bytes per format
     */
    inline auto getFormatBytesPerBlock(ResourceFormat format) -> uint32_t
    {
        AP_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].bytesPerBlock;
    }

    inline auto getFormatPixelsPerBlock(ResourceFormat format) -> uint32_t
    {
        AP_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compressionRatio.width * kFormatDesc[(uint32_t)format].compressionRatio.height;
    }

    /**
     * Check if the format has a depth component
     */
    inline auto isDepthFormat(ResourceFormat format) -> bool
    {
        AP_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isDepth;
    }

    /**
     * Check if the format has a stencil component
     */
    inline auto isStencilFormat(ResourceFormat format) -> bool
    {
        AP_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isStencil;
    }

    /**
     * Check if the format has depth or stencil components
     */
    inline auto isDepthStencilFormat(ResourceFormat format) -> bool
    {
        return isDepthFormat(format) || isStencilFormat(format);
    }

    /**
     * Check if the format is a compressed format
     */
    inline auto isCompressedFormat(ResourceFormat format) -> bool
    {
        AP_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isCompressed;
    }

    /**
     * Get the format compression ration along the x-axis
     */
    inline auto getFormatWidthCompressionRatio(ResourceFormat format) -> uint32_t
    {
        AP_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compressionRatio.width;
    }

    /**
     * Get the format compression ration along the y-axis
     */
    inline auto getFormatHeightCompressionRatio(ResourceFormat format) -> uint32_t
    {
        AP_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compressionRatio.height;
    }

    /**
     * Get the number of channels
     */
    inline auto getFormatChannelCount(ResourceFormat format) -> uint32_t
    {
        AP_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].channelCount;
    }

    /**
     * Get the format Type
     */
    inline auto getFormatType(ResourceFormat format) -> FormatType
    {
        AP_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].type;
    }

    /**
     * Check if a format is an integer type.
     */
    inline auto isIntegerFormat(ResourceFormat format) -> bool
    {
        FormatType type = getFormatType(format);
        return type == FormatType::Uint || type == FormatType::Sint;
    }

    /**
     * Get number of bits used for a given color channel.
     */
    inline auto getNumChannelBits(ResourceFormat format, int channel) -> uint32_t
    {
        return kFormatDesc[(uint32_t)format].numChannelBits[channel];
    }

    /**
     * Get number of bits used for the given color channels.
     */
    inline auto getNumChannelBits(ResourceFormat format, TextureChannelFlags mask) -> uint32_t
    {
        uint32_t bits = 0;
        if (enum_has_any_flags(mask, TextureChannelFlags::Red))   bits += getNumChannelBits(format, 0);
        if (enum_has_any_flags(mask, TextureChannelFlags::Green)) bits += getNumChannelBits(format, 1);
        if (enum_has_any_flags(mask, TextureChannelFlags::Blue))  bits += getNumChannelBits(format, 2);
        if (enum_has_any_flags(mask, TextureChannelFlags::Alpha)) bits += getNumChannelBits(format, 3);

        return bits;
    }

    /**
     * Get mask of enabled color channels. See TextureChannelFlags.
     */
    inline auto getChannelMask(ResourceFormat format) -> TextureChannelFlags
    {
        TextureChannelFlags mask = TextureChannelFlags::None;
        if (kFormatDesc[(uint32_t)format].numChannelBits[0] > 0) mask |= TextureChannelFlags::Red;
        if (kFormatDesc[(uint32_t)format].numChannelBits[1] > 0) mask |= TextureChannelFlags::Green;
        if (kFormatDesc[(uint32_t)format].numChannelBits[2] > 0) mask |= TextureChannelFlags::Blue;
        if (kFormatDesc[(uint32_t)format].numChannelBits[3] > 0) mask |= TextureChannelFlags::Alpha;

        return mask;
    }

    /**
     * Get the number of bytes per row. If format is compressed, width should be evenly divisible by the compression ratio.
     */
    inline auto getFormatRowPitch(ResourceFormat format, uint32_t width) -> uint32_t
    {
        AP_ASSERT(width % getFormatWidthCompressionRatio(format) == 0);
        return (width / getFormatWidthCompressionRatio(format)) * getFormatBytesPerBlock(format);
    }

    /**
     * Check if a format represents sRGB color space
     */
    inline auto isSrgbFormat(ResourceFormat format) -> bool
    {
        return (getFormatType(format) == FormatType::UnormSrgb);
    }

    /**
     * Convert an SRGB format to linear. If the format is already linear, will return it
     */
    inline auto srgbToLinearFormat(ResourceFormat format) -> ResourceFormat
    {
        switch (format)
        {
        case ResourceFormat::BC1UnormSrgb:
            return ResourceFormat::BC1Unorm;
        case ResourceFormat::BC2UnormSrgb:
            return ResourceFormat::BC2Unorm;
        case ResourceFormat::BC3UnormSrgb:
            return ResourceFormat::BC3Unorm;
        case ResourceFormat::BGRA8UnormSrgb:
            return ResourceFormat::BGRA8Unorm;
        case ResourceFormat::BGRX8UnormSrgb:
            return ResourceFormat::BGRX8Unorm;
        case ResourceFormat::RGBA8UnormSrgb:
            return ResourceFormat::RGBA8Unorm;
        case ResourceFormat::BC7UnormSrgb:
            return ResourceFormat::BC7Unorm;
        default:
            AP_ASSERT(isSrgbFormat(format) == false);
            return format;
        }
    }

    /**
     * Convert an linear format to sRGB. If the format doesn't have a matching sRGB format, will return the original
     */
    inline auto linearToSrgbFormat(ResourceFormat format) -> ResourceFormat
    {
        switch (format)
        {
        case ResourceFormat::BC1Unorm:
            return ResourceFormat::BC1UnormSrgb;
        case ResourceFormat::BC2Unorm:
            return ResourceFormat::BC2UnormSrgb;
        case ResourceFormat::BC3Unorm:
            return ResourceFormat::BC3UnormSrgb;
        case ResourceFormat::BGRA8Unorm:
            return ResourceFormat::BGRA8UnormSrgb;
        case ResourceFormat::BGRX8Unorm:
            return ResourceFormat::BGRX8UnormSrgb;
        case ResourceFormat::RGBA8Unorm:
            return ResourceFormat::RGBA8UnormSrgb;
        case ResourceFormat::BC7Unorm:
            return ResourceFormat::BC7UnormSrgb;
        default:
            return format;
        }
    }

    inline auto depthToColorFormat(ResourceFormat format) -> ResourceFormat
    {
        switch (format)
        {
        case ResourceFormat::D16Unorm:
            return ResourceFormat::R16Unorm;
        case ResourceFormat::D32Float:
            return ResourceFormat::R32Float;
        default:
            AP_ASSERT(isDepthFormat(format) == false);
            return format;
        }
    }

    inline auto doesFormatHaveAlpha(ResourceFormat format) -> bool
    {
        if (getFormatChannelCount(format) == 4)
        {
            switch (format)
            {
            case ResourceFormat::BGRX8Unorm:
            case ResourceFormat::BGRX8UnormSrgb:
                return false;
            default:
                return true;
            }
        }

        return false;
    }

    inline auto to_string(ResourceFormat format) -> std::string const&
    {
        AP_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].name;
    }

    inline auto to_string(FormatType type) -> std::string const
    {
    #define item_to_string(item) \
        case FormatType::item:  \
            return #item;
        switch (type)
        {
            item_to_string(Unknown);
            item_to_string(Float);
            item_to_string(Unorm);
            item_to_string(UnormSrgb);
            item_to_string(Snorm);
            item_to_string(Uint);
            item_to_string(Sint);
        default:
            AP_UNREACHABLE();
        }
    #undef item_to_string
    }

    struct ResourceFormat_info
    {
        static auto items() -> std::span<std::pair<ResourceFormat, std::string>>
        {
            auto createItems = []()
            {
                std::vector<std::pair<ResourceFormat, std::string>> items((size_t)ResourceFormat::Count);
                for (size_t i = 0; i < (size_t)ResourceFormat::Count; ++i)
                    items[i] = std::make_pair(ResourceFormat(i), to_string(ResourceFormat(i)));
                return items;
            };
            static std::vector<std::pair<ResourceFormat, std::string>> items = createItems();
            return items;
        }
    };
    AP_ENUM_REGISTER(ResourceFormat);

} // namespace april::graphics
