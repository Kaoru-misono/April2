// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "fwd.hpp"
#include "native-handle.hpp"
#include "program/program-reflection.hpp"

#include <core/foundation/object.hpp>
#include <slang-com-ptr.h>
#include <slang-rhi.h>

namespace april::graphics
{
    class Device;
    class Resource;
    class Texture;
    class Buffer;

    struct ResourceViewInfo
    {
        ResourceViewInfo() = default;
        ResourceViewInfo(uint32_t mostDetailedMip, uint32_t mipCount, uint32_t firstArraySlice, uint32_t arraySize)
            : mostDetailedMip(mostDetailedMip), mipCount(mipCount), firstArraySlice(firstArraySlice), arraySize(arraySize)
        {}

        ResourceViewInfo(uint64_t offset, uint64_t size) : offset(offset), size(size) {}

        static constexpr uint32_t kMaxPossible = uint32_t(-1);
        static constexpr uint64_t kEntireBuffer = uint64_t(-1);

        // Textures
        uint32_t mostDetailedMip{};
        uint32_t mipCount{kMaxPossible};
        uint32_t firstArraySlice{};
        uint32_t arraySize{kMaxPossible};

        // Buffers
        uint64_t offset{};
        uint64_t size{kEntireBuffer};

        auto operator==(ResourceViewInfo const& other) const -> bool
        {
            return (firstArraySlice == other.firstArraySlice) && (arraySize == other.arraySize) && (mipCount == other.mipCount) &&
                   (mostDetailedMip == other.mostDetailedMip) && (offset == other.offset) && (size == other.size);
        }
    };

    /**
     * Abstracts API resource views.
     */
    class ResourceView : public core::Object
    {
        APRIL_OBJECT(ResourceView)
    public:
        using Dimension = ReflectionResourceType::Dimensions;

        static uint32_t const kMaxPossible = uint32_t(-1);
        static constexpr uint64_t kEntireBuffer = uint64_t(-1);
        virtual ~ResourceView() = default;

        ResourceView(
            Device* pDevice,
            Resource* pResource,
            rhi::ComPtr<rhi::ITextureView> pTextureView,
            uint32_t mostDetailedMip,
            uint32_t mipCount,
            uint32_t firstArraySlice,
            uint32_t arraySize
        );

        ResourceView(Device* pDevice, Resource* pResource, rhi::ComPtr<rhi::IBuffer> pBuffer, uint64_t offset, uint64_t size);

        auto getGfxBinding() const -> rhi::Binding;

        auto getGfxTexture() const -> rhi::ITexture*;
        auto getGfxBuffer() const -> rhi::IBuffer*;
        auto getGfxTextureView() const -> rhi::ITextureView*;

        auto getNativeHandle() const -> NativeHandle { return NativeHandle(); }
        auto getResource() const -> Resource* { return mp_resource; }

        /**
         * Get information about the view.
         */
        auto getViewInfo() const -> ResourceViewInfo const& { return m_viewInfo; }

    protected:
        friend class Resource;

        auto invalidate() -> void;

        Device* mp_device{};
        Resource* mp_resource{};
        enum struct ResourceType: uint8_t
        {
            None,
            Buffer,
            Texture,
        };
        ResourceType m_type{};
        ResourceViewInfo m_viewInfo{};
        rhi::ComPtr<rhi::ITextureView> m_gfxTextureView{};
        rhi::ComPtr<rhi::IBuffer> mp_gfxBuffer{};
    };

    class ShaderResourceView : public ResourceView
    {
    public:
        using Dimension = ResourceView::Dimension;

        static auto create(
            Device* pDevice,
            Texture* pTexture,
            uint32_t mostDetailedMip,
            uint32_t mipCount,
            uint32_t firstArraySlice,
            uint32_t arraySize
        ) -> core::ref<ShaderResourceView>;

        static auto create(Device* pDevice, Buffer* pBuffer, uint64_t offset, uint64_t size) -> core::ref<ShaderResourceView>;

    private:
        ShaderResourceView(
            Device* pDevice,
            Resource* pResource,
            Slang::ComPtr<rhi::ITextureView> gfxResourceView,
            uint32_t mostDetailedMip,
            uint32_t mipCount,
            uint32_t firstArraySlice,
            uint32_t arraySize
        );
        ShaderResourceView(
            Device* pDevice,
            Resource* pResource,
            Slang::ComPtr<rhi::IBuffer> gfxBuffer,
            uint64_t offset,
            uint64_t size
        );
    };

    class DepthStencilView : public ResourceView
    {
    public:
        static auto create(
            Device* pDevice,
            Texture* pTexture,
            uint32_t mipLevel,
            uint32_t firstArraySlice,
            uint32_t arraySize
        ) -> core::ref<DepthStencilView>;

    private:
        DepthStencilView(
            Device* pDevice,
            Resource* pResource,
            Slang::ComPtr<rhi::ITextureView> gfxResourceView,
            uint32_t mipLevel,
            uint32_t firstArraySlice,
            uint32_t arraySize
        );
    };

    class UnorderedAccessView : public ResourceView
    {
    public:
        using Dimension = ResourceView::Dimension;

        static auto create(
            Device* pDevice,
            Texture* pTexture,
            uint32_t mipLevel,
            uint32_t firstArraySlice,
            uint32_t arraySize
        ) -> core::ref<UnorderedAccessView>;
        static auto create(Device* pDevice, Buffer* pBuffer, uint64_t offset, uint64_t size) -> core::ref<UnorderedAccessView>;

    private:
        UnorderedAccessView(
            Device* pDevice,
            Resource* pResource,
            Slang::ComPtr<rhi::ITextureView> gfxResourceView,
            uint32_t mipLevel,
            uint32_t firstArraySlice,
            uint32_t arraySize
        );

        UnorderedAccessView(
            Device* pDevice,
            Resource* pResource,
            Slang::ComPtr<rhi::IBuffer> gfxBuffer,
            uint64_t offset,
            uint64_t size
        );
    };

    class RenderTargetView : public ResourceView
    {
    public:
         using Dimension = ResourceView::Dimension;

        static auto create(
            Device* pDevice,
            Texture* pTexture,
            uint32_t mipLevel,
            uint32_t firstArraySlice,
            uint32_t arraySize
        ) -> core::ref<RenderTargetView>;

    private:
        RenderTargetView(
            Device* pDevice,
            Resource* pResource,
            Slang::ComPtr<rhi::ITextureView> gfxResourceView,
            uint32_t mipLevel,
            uint32_t firstArraySlice,
            uint32_t arraySize
        );
    };
} // namespace april::graphics
