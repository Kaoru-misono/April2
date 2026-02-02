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

    /**
     * Vertex attribute flags for mesh binary format.
     * Indicates which optional attributes are present in vertex data.
     */
    enum VertexFlags : uint8_t
    {
        HasTexCoord1 = 1 << 0,  // Secondary UV channel (lightmap, etc.)
        HasColor     = 1 << 1,  // Vertex color (RGBA float)
    };

    /**
     * Submesh definition within a mesh asset.
     * Represents a contiguous range of indices with an associated material.
     */
    struct Submesh
    {
        uint32_t indexOffset = 0;     // Offset in index buffer
        uint32_t indexCount = 0;      // Number of indices
        uint32_t materialIndex = 0;   // Material slot
    };

    /**
     * Standard layout header for compiled mesh blobs.
     * Binary format: [MeshHeader][Submesh[]...][vertex data...][index data...]
     */
    struct MeshHeader
    {
        static constexpr uint32_t kMagic = 0x41504D58; // "APMX" - April Mesh

        uint32_t magic = kMagic;
        uint32_t version = 1;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint32_t vertexStride = 0;      // 48+ bytes based on flags
        uint32_t indexFormat = 0;       // 0=uint16, 1=uint32
        uint32_t submeshCount = 0;
        uint32_t flags = 0;             // VertexFlags
        float boundsMin[3] = {0, 0, 0}; // AABB min
        float boundsMax[3] = {0, 0, 0}; // AABB max
        uint64_t vertexDataSize = 0;
        uint64_t indexDataSize = 0;
        uint64_t reserved = 0;          // Reserved for future use (padding to 80 bytes)

        [[nodiscard]] auto isValid() const -> bool
        {
            return magic == kMagic && version == 1 && vertexCount > 0 && indexCount > 0;
        }
    };

    static_assert(sizeof(MeshHeader) == 80, "MeshHeader must be 80 bytes for binary compatibility");

    /**
     * Mesh data payload returned from AssetManager.
     * Holds the parsed header and views into submesh, vertex, and index data.
     */
    struct MeshPayload
    {
        MeshHeader header{};
        std::span<Submesh const> submeshes{};
        std::span<std::byte const> vertexData{};
        std::span<std::byte const> indexData{};

        [[nodiscard]] auto isValid() const -> bool
        {
            return header.isValid() && !vertexData.empty() && !indexData.empty();
        }
    };

} // namespace april::asset
