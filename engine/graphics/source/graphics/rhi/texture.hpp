// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "resource.hpp"
#include "resource-views.hpp"
#include "tools/bit-map.hpp"

#include <core/math/type.hpp>

#include <filesystem>
#include <algorithm>
// #include "utils/image/bitmap.hpp" // Placeholder if exists

namespace april::graphics
{
    class Sampler;
    class CommandContext;

    enum class TextureUsage: uint32_t
    {
        None = 0,
        ShaderResource = (1 << 0),
        UnorderedAccess = (1 << 1),
        RenderTarget = (1 << 2),
        DepthStencil = (1 << 3),
        Present = (1 << 4),
        CopySource = (1 << 5),
        CopyDestination = (1 << 6),
        ResolveSource = (1 << 7),
        ResolveDestination = (1 << 8),
        Typeless = (1 << 9),
        Shared = (1 << 10),
    };
    AP_ENUM_CLASS_OPERATORS(TextureUsage);

    /**
     * Abstracts the API texture objects
     */
    class Texture : public Resource
    {
        APRIL_OBJECT(Texture)
    public:
        struct SubresourceLayout
        {
            /// Size of a single row in bytes (unaligned).
            size_t rowSize;
            /// Size of a single row in bytes (aligned to device texture alignment).
            size_t rowSizeAligned;
            /// Number of rows.
            size_t rowCount;
            /// Number of depth slices.
            size_t depth;

            /// Get the total size of the subresource in bytes (unaligned).
            auto getTotalByteSize() const -> size_t { return rowSize * rowCount * depth; }

            /// Get the total size of the subresource in bytes (aligned to device texture alignment).
            auto getTotalByteSizeAligned() const -> size_t { return rowSizeAligned * rowCount * depth; }
        };

        Texture(
            core::ref<Device> p_device,
            Type type,
            ResourceFormat format,
            uint32_t width,
            uint32_t height,
            uint32_t depth,
            uint32_t arraySize,
            uint32_t mipLevels,
            uint32_t sampleCount,
            TextureUsage usage,
            void const* pInitData
        );

        Texture(
            core::ref<Device> p_device,
            rhi::ITexture* pResource,
            Type type,
            ResourceFormat format,
            uint32_t width,
            uint32_t height,
            uint32_t depth,
            uint32_t arraySize,
            uint32_t mipLevels,
            uint32_t sampleCount,
            TextureUsage usage,
            Resource::State initState
        );

        virtual ~Texture();

        /**
         * Get a mip-level width
         */
        auto getWidth(uint32_t mipLevel = 0) const -> uint32_t
        {
            return (mipLevel == 0) || (mipLevel < m_mipLevels) ? std::max(1U, m_width >> mipLevel) : 0;
        }

        /**
         * Get a mip-level height
         */
        auto getHeight(uint32_t mipLevel = 0) const -> uint32_t
        {
            return (mipLevel == 0) || (mipLevel < m_mipLevels) ? std::max(1U, m_height >> mipLevel) : 0;
        }

        /**
         * Get a mip-level depth
         */
        auto getDepth(uint32_t mipLevel = 0) const -> uint32_t
        {
            return (mipLevel == 0) || (mipLevel < m_mipLevels) ? std::max(1U, m_depth >> mipLevel) : 0;
        }

        /**
         * Get the number of mip-levels
         */
        auto getMipCount() const -> uint32_t { return m_mipLevels; }

        /**
         * Get the sample count
         */
        auto getSampleCount() const -> uint32_t { return m_sampleCount; }

        /**
         * Get the array size
         */
        auto getArraySize() const -> uint32_t { return m_arraySize; }

        /**
         * Get the array index of a subresource
         */
        auto getSubresourceArraySlice(uint32_t subresource) const -> uint32_t { return subresource / m_mipLevels; }

        /**
         * Get the mip-level of a subresource
         */
        auto getSubresourceMipLevel(uint32_t subresource) const -> uint32_t { return subresource % m_mipLevels; }

        /**
         * Get the subresource index
         */
        auto getSubresourceIndex(uint32_t arraySlice, uint32_t mipLevel) const -> uint32_t { return mipLevel + arraySlice * m_mipLevels; }

        /**
         * Get the number of subresources
         */
        auto getSubresourceCount() const -> uint32_t { return m_mipLevels * m_arraySize; }

        /**
         * Get the texture usage
         */
        auto getUsage() const -> TextureUsage { return m_usage; }

        /**
         * Get the resource format
         */
        auto getFormat() const -> ResourceFormat { return m_format; }

        auto createMippedFromFiles(
            core::ref<Device> pDevice,
            std::span<const std::filesystem::path> paths,
            bool loadAsSrgb,
            TextureUsage usage = TextureUsage::ShaderResource,
            Bitmap::ImportFlags importFlags = Bitmap::ImportFlags::None
        ) -> core::ref<Texture>;

        auto createFromFile(
            core::ref<Device> pDevice,
            std::filesystem::path const& path,
            bool generateMipLevels,
            bool loadAsSrgb,
            TextureUsage usage = TextureUsage::ShaderResource,
            Bitmap::ImportFlags importFlags = Bitmap::ImportFlags::None
        );

        auto getGfxTextureResource() const -> rhi::ITexture* { return m_gfxTexture; }

        auto getGfxResource() const -> rhi::IResource* override;

        /**
         * Invalidate and release all texture views.
         */
        auto invalidateViews() const -> void override;

        /**
         * Get a shader-resource view for the entire resource.
         */
        auto getSRV() -> core::ref<TextureView> { return getSRV(0); }

        /**
         * Get an unordered access view for the entire resource.
         */
        auto getUAV() -> core::ref<TextureView> { return getUAV(0); }

        /**
         * Get a shader-resource view.
         */
        auto getSRV(
            uint32_t mostDetailedMip,
            uint32_t mipCount = kMaxPossible,
            uint32_t firstArraySlice = 0,
            uint32_t arraySize = kMaxPossible
        ) -> core::ref<TextureView>;

        /**
         * Get a render-target view.
         */
        auto getRTV(uint32_t mipLevel = 0, uint32_t firstArraySlice = 0, uint32_t arraySize = kMaxPossible) -> core::ref<TextureView>;

        /**
         * Get a depth stencil view.
         */
        auto getDSV(uint32_t mipLevel = 0, uint32_t firstArraySlice = 0, uint32_t arraySize = kMaxPossible) -> core::ref<TextureView>;

        /**
         * Get an unordered access view.
         */
        auto getUAV(uint32_t mipLevel, uint32_t firstArraySlice = 0, uint32_t arraySize = kMaxPossible) -> core::ref<TextureView>;

        /**
         * Get the data layout of a subresource.
         * @param[in] subresource The subresource index.
         */
        auto getSubresourceLayout(uint32_t subresource) const -> SubresourceLayout;

        /**
         * Set the data of a subresource.
         * @param[in] subresource The subresource index.
         * @param[in] pData The data to write.
         * @param[in] size The size of the data (must match the actual subresource size).
         */
        auto setSubresourceBlob(uint32_t subresource, void const* pData, size_t size) -> void;

        /**
         * Get the data of a subresource.
         * @param[in] subresource The subresource index.
         * @param[in] pData The data buffer to read to.
         * @param[in] size The size of the data (must match the actual subresource size).
         */
        auto getSubresourceBlob(uint32_t subresource, void* pData, size_t size) const -> void;

        auto captureToFile(
            uint32_t mipLevel,
            uint32_t arraySlice,
            std::filesystem::path const& path,
            Bitmap::FileFormat format = Bitmap::FileFormat::PngFile,
            Bitmap::ExportFlags exportFlags = Bitmap::ExportFlags::None,
            bool async = true
        );

        /**
         * Generates mipmaps for a specified texture object.
         * @param[in] pContext Used command context.
         * @param[in] minMaxMips Generate a min/max mipmap pyramid.
         */
        auto generateMips(CommandContext* pContext, bool minMaxMips = false) -> void;

        /**
         * In case the texture was loaded from a file, use this to set the file path
         */
        auto setSourcePath(std::filesystem::path const& path) -> void { m_sourcePath = path; }

        /**
         * In case the texture was loaded from a file, get the source file path
         */
        auto getSourcePath() const -> std::filesystem::path const& { return m_sourcePath; }

        /**
         * In case the texture was loaded from a file, get the import flags used.
         */
        auto getImportFlags() const -> Bitmap::ImportFlags { return m_importFlags; }

        /**
         * Returns the total number of texels across all mip levels and array slices.
         */
        auto getTexelCount() const -> uint64_t;

        /**
         * Returns the size of the texture in bytes as allocated in GPU memory.
         */
        auto getTextureSizeInBytes() const -> uint64_t;

        /**
         * Compares the texture description to another texture.
         * @return True if all fields (size/format/etc) are identical.
         */
        auto compareDesc(Texture const* pOther) const -> bool;

    protected:
        friend class Device;

        auto uploadInitData(CommandContext* pCommandContext, void const* pData, bool autoGenMips) -> void;

        rhi::ComPtr<rhi::ITexture> m_gfxTexture;

        bool m_releaseRtvsAfterGenMips{true};
        std::filesystem::path m_sourcePath;
        Bitmap::ImportFlags m_importFlags = Bitmap::ImportFlags::None;
        TextureUsage m_usage{TextureUsage::None};

        ResourceFormat m_format{ResourceFormat::Unknown};
        uint32_t m_width{0};
        uint32_t m_height{0};
        uint32_t m_depth{0};
        uint32_t m_mipLevels{0};
        uint32_t m_arraySize{0};
        uint32_t m_sampleCount{0};
        bool m_isSparse{false};
        int3 m_sparsePageRes{0};

        // View caches
        mutable std::unordered_map<ResourceViewInfo, core::ref<TextureView>, ViewInfoHashFunc> m_srvs;
        mutable std::unordered_map<ResourceViewInfo, core::ref<TextureView>, ViewInfoHashFunc> m_rtvs;
        mutable std::unordered_map<ResourceViewInfo, core::ref<TextureView>, ViewInfoHashFunc> m_dsvs;
        mutable std::unordered_map<ResourceViewInfo, core::ref<TextureView>, ViewInfoHashFunc> m_uavs;
    };

    inline auto to_string(TextureUsage usages) -> std::string
    {
        std::string s;
        if (usages == TextureUsage::None)
        {
            return "None";
        }

    #define item_to_string(item)                       \
        if (enum_has_any_flags(usages, TextureUsage::item)) \
        (s += (s.size() ? " | " : "") + std::string(#item))

        item_to_string(ShaderResource);
        item_to_string(UnorderedAccess);
        item_to_string(RenderTarget);
        item_to_string(DepthStencil);
        item_to_string(Present);
        item_to_string(CopySource);
        item_to_string(CopyDestination);
        item_to_string(ResolveSource);
        item_to_string(ResolveDestination);
        item_to_string(Typeless);
        item_to_string(Shared);

    #undef item_to_string

        return s;
    }
} // namespace april::graphics
