// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "fwd.hpp"
#include "native-handle.hpp"
#include "format.hpp"
#include <core/foundation/object.hpp>
#include <core/tools/enum-flags.hpp>

#include <slang-com-ptr.h>
#include <slang-rhi.h>

namespace april::graphics
{
    class Device;
    class Resource;
    class Texture;
    class Buffer;

    /**
     * Defines how the resource view is bound to the pipeline.
     */
    enum class ResourceBindFlags : uint32_t
    {
        None            = 0,
        ShaderResource  = 1 << 0, // SRV (t register)
        UnorderedAccess = 1 << 1, // UAV (u register)
        RenderTarget    = 1 << 2, // RTV
        DepthStencil    = 1 << 3  // DSV
    };
    AP_ENUM_CLASS_OPERATORS(ResourceBindFlags);

    /**
     * Defines the topological dimension of the view.
     */
    enum class ResourceViewDimension : uint32_t
    {
        Undefined = 0,
        Buffer,
        Texture1D,
        Texture1DArray,
        Texture2D,
        Texture2DArray,
        Texture2DMS,
        Texture2DMSArray,
        Texture3D,
        TextureCube,
        TextureCubeArray,
    };

    /**
     * Defines the interpretation mode for buffers.
     */
    enum class BufferCreationMode : uint32_t
    {
        Typed,      // Buffer<T>, depends on Format
        Structured, // StructuredBuffer<T>, depends on Stride
        Raw         // ByteAddressBuffer / Raw Buffer
    };

    // =====================================================
    // ResourceViewInfo - For view caching in Resource class
    // =====================================================

    struct ResourceViewInfo
    {
        static constexpr uint32_t kMaxPossible = uint32_t(-1);
        static constexpr uint64_t kEntireBuffer = uint64_t(-1);

        ResourceViewInfo() = default;

        // For texture views
        ResourceViewInfo(uint32_t mostDetailedMip, uint32_t mipCount, uint32_t firstArraySlice, uint32_t arraySize)
            : mostDetailedMip(mostDetailedMip), mipCount(mipCount), firstArraySlice(firstArraySlice), arraySize(arraySize)
        {}

        // For buffer views
        ResourceViewInfo(uint64_t offset, uint64_t size)
            : offset(offset), size(size)
        {}

        auto operator==(ResourceViewInfo const& other) const -> bool
        {
            return mostDetailedMip == other.mostDetailedMip &&
                   mipCount == other.mipCount &&
                   firstArraySlice == other.firstArraySlice &&
                   arraySize == other.arraySize &&
                   offset == other.offset &&
                   size == other.size;
        }

        uint32_t mostDetailedMip{0};
        uint32_t mipCount{kMaxPossible};
        uint32_t firstArraySlice{0};
        uint32_t arraySize{kMaxPossible};
        uint64_t offset{0};
        uint64_t size{kEntireBuffer};
    };

    /**
     * Universal descriptor for creating any type of resource view.
     */
    struct ResourceViewDesc
    {
        ResourceBindFlags bindFlags{ResourceBindFlags::None};
        ResourceViewDimension dimension{ResourceViewDimension::Undefined};

        ResourceFormat format{ResourceFormat::Unknown};

        struct TextureInfo
        {
            uint32_t mostDetailedMip{0};
            uint32_t mipCount{uint32_t(-1)};
            uint32_t firstArraySlice{0};
            uint32_t arraySize{uint32_t(-1)};
        } texture;

        struct BufferInfo
        {
            uint64_t offset{0};
            uint64_t size{uint64_t(-1)};
            BufferCreationMode mode{BufferCreationMode::Typed};
            uint32_t elementStride{0}; // Required for StructuredBuffer
        } buffer;

        // --- Fluent API Helpers ---

        static auto AsStructuredBuffer(uint32_t stride, uint64_t offset = 0, uint64_t size = uint64_t(-1)) -> ResourceViewDesc
        {
            ResourceViewDesc d;
            d.dimension = ResourceViewDimension::Buffer;
            d.bindFlags = ResourceBindFlags::ShaderResource;
            d.format = ResourceFormat::Unknown;
            d.buffer.mode = BufferCreationMode::Structured;
            d.buffer.elementStride = stride;
            d.buffer.offset = offset;
            d.buffer.size = size;
            return d;
        }

        static auto AsRWStructuredBuffer(uint32_t stride, uint64_t offset = 0, uint64_t size = uint64_t(-1)) -> ResourceViewDesc
        {
            auto d = AsStructuredBuffer(stride, offset, size);
            d.bindFlags = ResourceBindFlags::UnorderedAccess;
            return d;
        }

        static auto AsRawBuffer(uint64_t offset = 0, uint64_t size = uint64_t(-1)) -> ResourceViewDesc
        {
            ResourceViewDesc d;
            d.dimension = ResourceViewDimension::Buffer;
            d.bindFlags = ResourceBindFlags::ShaderResource;
            d.format = ResourceFormat::R32Float;
            d.buffer.mode = BufferCreationMode::Raw;
            d.buffer.offset = offset;
            d.buffer.size = size;
            return d;
        }

        static auto AsRenderTarget(ResourceFormat format, uint32_t mipLevel = 0) -> ResourceViewDesc
        {
            ResourceViewDesc d;
            d.dimension = ResourceViewDimension::Texture2D;
            d.bindFlags = ResourceBindFlags::RenderTarget;
            d.format = format;
            d.texture.mostDetailedMip = mipLevel;
            d.texture.mipCount = 1;
            return d;
        }

        static auto AsDepthStencil(ResourceFormat format, uint32_t mipLevel = 0) -> ResourceViewDesc
        {
            ResourceViewDesc d;
            d.dimension = ResourceViewDimension::Texture2D;
            d.bindFlags = ResourceBindFlags::DepthStencil;
            d.format = format;
            d.texture.mostDetailedMip = mipLevel;
            d.texture.mipCount = 1;
            return d;
        }
    };

    /**
     * Abstract base class for all resource views.
     */
    class ResourceView : public core::Object
    {
        APRIL_OBJECT(ResourceView)
    public:
        virtual ~ResourceView() = default;

        /**
         * Get the underlying resource associated with this view.
         */
        virtual auto getResource() const -> Resource* = 0;

        /**
         * Get the native API handle (if applicable).
         */
        virtual auto getNativeHandle() const -> NativeHandle { return {}; }

        /**
         * Get the RHI binding for use with shader objects.
         */
        virtual auto getGfxBinding() const -> rhi::Binding = 0;

        /**
         * Get the descriptor used to create this view.
         */
        auto getDesc() const -> ResourceViewDesc const& { return m_desc; }

        /**
         * Helper check: Is this view bound as a UAV?
         */
        auto isUAV() const -> bool { return (m_desc.bindFlags & ResourceBindFlags::UnorderedAccess) != ResourceBindFlags::None; }

        /**
         * Helper check: Is this view bound as an SRV?
         */
        auto isSRV() const -> bool { return (m_desc.bindFlags & ResourceBindFlags::ShaderResource) != ResourceBindFlags::None; }

        /**
         * Helper check: Is this view an RTV?
         */
        auto isRTV() const -> bool { return (m_desc.bindFlags & ResourceBindFlags::RenderTarget) != ResourceBindFlags::None; }

        /**
         * Helper check: Is this view a DSV?
         */
        auto isDSV() const -> bool { return (m_desc.bindFlags & ResourceBindFlags::DepthStencil) != ResourceBindFlags::None; }

        /**
         * Get view info for caching/comparison purposes.
         * Returns a ResourceViewInfo compatible with the old API.
         */
        auto getViewInfo() const -> ResourceViewInfo
        {
            if (m_desc.dimension == ResourceViewDimension::Buffer)
            {
                return ResourceViewInfo(m_desc.buffer.offset, m_desc.buffer.size);
            }
            else
            {
                return ResourceViewInfo(
                    m_desc.texture.mostDetailedMip,
                    m_desc.texture.mipCount,
                    m_desc.texture.firstArraySlice,
                    m_desc.texture.arraySize
                );
            }
        }

    protected:
        ResourceView(Device* pDevice, ResourceViewDesc const& desc)
            : mp_device(pDevice)
            , m_desc(desc)
        {}

        auto invalidate() -> void
        {
            // Logic to handle resource destruction notification
            // e.g. set internal RHI pointers to null
        }

        Device* mp_device{nullptr};
        ResourceViewDesc m_desc;
    };

    /**
     * A view into a Texture resource.
     * Can represent SRV, UAV, RTV, or DSV depending on the descriptor.
     */
    class TextureView : public ResourceView
    {
        APRIL_OBJECT(TextureView)
    public:
        static auto create(Device* pDevice, Texture* pTexture, ResourceViewDesc const& desc) -> core::ref<TextureView>;

        auto getResource() const -> Resource* override;
        auto getGfxBinding() const -> rhi::Binding override { return rhi::Binding{m_gfxTextureView.get()}; }

        /**
         * Get the underlying RHI TextureView.
         */
        auto getGfxTextureView() const -> rhi::ITextureView* { return m_gfxTextureView.get(); }

        /**
         * Get the underlying RHI Texture from the view.
         */
        auto getGfxTexture() const -> rhi::ITexture* { return m_gfxTextureView ? m_gfxTextureView->getTexture() : nullptr; }

        /**
         * Get the Texture this view references.
         */
        auto getTexture() const -> Texture* { return mp_texture; }

    private:
        TextureView(Device* pDevice, Texture* pTexture, Slang::ComPtr<rhi::ITextureView> gfxTextureView, ResourceViewDesc const& desc);

        Texture* mp_texture{nullptr};
        Slang::ComPtr<rhi::ITextureView> m_gfxTextureView;
    };

    /**
     * A view into a Buffer resource.
     * Handles Typed, Structured, and Raw buffer views.
     */
    class BufferView : public ResourceView
    {
        APRIL_OBJECT(BufferView)
    public:
        static auto create(Device* pDevice, Buffer* pBuffer, ResourceViewDesc const& desc) -> core::ref<BufferView>;

        auto getResource() const -> Resource* override;
        auto getGfxBinding() const -> rhi::Binding override;

        /**
         * Get the underlying RHI Buffer (or BufferView if applicable).
         */
        auto getGfxBuffer() const -> rhi::IBuffer* { return m_gfxBuffer.get(); }

        /**
         * Get the Buffer this view references.
         */
        auto getBuffer() const -> Buffer* { return mp_buffer; }

    private:
        BufferView(Device* pDevice, Buffer* pBuffer, Slang::ComPtr<rhi::IBuffer> gfxBuffer, ResourceViewDesc const& desc);

        Buffer* mp_buffer{nullptr};
        Slang::ComPtr<rhi::IBuffer> m_gfxBuffer;
    };

} // namespace april::graphics
