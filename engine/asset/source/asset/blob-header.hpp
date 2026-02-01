#pragma once

#include <cstdint>
#include <cstddef>
#include <span>

namespace april::asset
{
    /**
     * Pixel format for compiled texture blobs.
     * Values match graphics::ResourceFormat for easy conversion.
     */
    enum struct PixelFormat : uint32_t
    {
        Unknown = 0,
        R8Unorm = 1,
        RG8Unorm = 5,
        RGBA8Unorm = 8,
        RGBA8UnormSrgb = 13,
    };

    /**
     * Standard layout header for compiled texture blobs.
     * Binary format: [TextureHeader][pixel data...]
     */
    struct TextureHeader
    {
        static constexpr uint32_t kMagic = 0x41505458; // "APTX" - April Texture

        uint32_t magic = kMagic;
        uint32_t version = 1;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 0;
        PixelFormat format = PixelFormat::Unknown;
        uint32_t mipLevels = 1;
        uint32_t flags = 0;         // Reserved for future use (e.g., sRGB flag)
        uint64_t dataSize = 0;      // Size of pixel data in bytes

        [[nodiscard]] auto isValid() const -> bool
        {
            return magic == kMagic && version == 1 && width > 0 && height > 0;
        }
    };

    static_assert(sizeof(TextureHeader) == 40, "TextureHeader must be 40 bytes for binary compatibility");

    /**
     * Texture data payload returned from AssetManager.
     * Holds the parsed header and a view into the pixel data.
     */
    struct TexturePayload
    {
        TextureHeader header{};
        std::span<std::byte const> pixelData{};

        [[nodiscard]] auto isValid() const -> bool
        {
            return header.isValid() && !pixelData.empty();
        }
    };

} // namespace april::asset
