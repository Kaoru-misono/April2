// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "resource-views.hpp"
#include "resource.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "render-device.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>
#include <core/tools/enum-flags.hpp>

namespace april::graphics
{
    ResourceView::ResourceView(
        Device* pDevice,
        Resource* pResource,
        rhi::ComPtr<rhi::ITextureView> pTextureView,
        uint32_t mostDetailedMip,
        uint32_t mipCount,
        uint32_t firstArraySlice,
        uint32_t arraySize
    )
        : mp_device(pDevice)
        , mp_resource(pResource)
        , m_type(ResourceType::Texture)
        , m_viewInfo(mostDetailedMip, mipCount, firstArraySlice, arraySize)
        , m_gfxTextureView(pTextureView)
    {}

    ResourceView::ResourceView(Device* pDevice, Resource* pResource, rhi::ComPtr<rhi::IBuffer> pBuffer, uint64_t offset, uint64_t size)
        : mp_device(pDevice), mp_resource(pResource), m_type(ResourceType::Buffer), m_viewInfo(offset, size), mp_gfxBuffer(pBuffer)
    {}

    auto ResourceView::getGfxBinding() const -> rhi::Binding
    {
        if (m_type == ResourceType::Buffer)
        {
            return rhi::Binding{mp_gfxBuffer.get(), rhi::BufferRange{m_viewInfo.offset, m_viewInfo.size}};
        }
        else if (m_type == ResourceType::Texture)
        {
            return rhi::Binding{m_gfxTextureView.get()};
        }

        AP_UNREACHABLE();
    }

    auto ResourceView::getGfxTexture() const -> rhi::ITexture* { AP_ASSERT(m_gfxTextureView); return m_gfxTextureView->getTexture(); }
    auto ResourceView::getGfxBuffer() const -> rhi::IBuffer* { AP_ASSERT(mp_gfxBuffer); return mp_gfxBuffer.get(); }
    auto ResourceView::getGfxTextureView() const -> rhi::ITextureView* { return m_gfxTextureView.get(); }

    auto ResourceView::invalidate() -> void
    {
        if (mp_device)
        {
            m_gfxTextureView = nullptr;
            mp_gfxBuffer = nullptr;
            mp_device = nullptr;
        }
    }

    // ShaderResourceView

    ShaderResourceView::ShaderResourceView(
        Device* pDevice,
        Resource* pResource,
        Slang::ComPtr<rhi::ITextureView> gfxResourceView,
        uint32_t mostDetailedMip,
        uint32_t mipCount,
        uint32_t firstArraySlice,
        uint32_t arraySize
    )
        : ResourceView(pDevice, pResource, gfxResourceView, mostDetailedMip, mipCount, firstArraySlice, arraySize)
    {}

    ShaderResourceView::ShaderResourceView(
        Device* pDevice,
        Resource* pResource,
        Slang::ComPtr<rhi::IBuffer> gfxBuffer,
        uint64_t offset,
        uint64_t size
    )
        : ResourceView(pDevice, pResource, gfxBuffer, offset, size)
    {}

    auto ShaderResourceView::create(
        Device* pDevice,
        Texture* pTexture,
        uint32_t mostDetailedMip,
        uint32_t mipCount,
        uint32_t firstArraySlice,
        uint32_t arraySize
    ) -> core::ref<ShaderResourceView>
    {
        AP_ASSERT(enum_has_any_flags(pTexture->getBindFlags(), ResourceBindFlags::ShaderResource));
        rhi::ComPtr<rhi::ITextureView> handle{};
        rhi::TextureViewDesc desc = {};
        desc.format = getGFXFormat(depthToColorFormat(pTexture->getFormat()));
        desc.aspect = rhi::TextureAspect::All;
        desc.subresourceRange.layer = firstArraySlice;
        desc.subresourceRange.layerCount = arraySize;
        desc.subresourceRange.mip = mostDetailedMip;
        desc.subresourceRange.mipCount = mipCount;
        auto debugName = pTexture->getName() + "SRV";
        desc.label = debugName.c_str();
        checkResult(pTexture->getGfxTextureResource()->createView(desc, handle.writeRef()), "Failed to create texture SRV");
        return core::ref<ShaderResourceView>(new ShaderResourceView(pDevice, pTexture, handle, mostDetailedMip, mipCount, firstArraySlice, arraySize));
    }

    auto ShaderResourceView::create(Device* pDevice, Buffer* pBuffer, uint64_t offset, uint64_t size) -> core::ref<ShaderResourceView>
    {
        return core::ref<ShaderResourceView>(new ShaderResourceView(pDevice, pBuffer, rhi::ComPtr<rhi::IBuffer>{pBuffer->getGfxBufferResource()}, offset, size));
    }

    // DepthStencilView

    DepthStencilView::DepthStencilView(
        Device* pDevice,
        Resource* pResource,
        Slang::ComPtr<rhi::ITextureView> gfxResourceView,
        uint32_t mipLevel,
        uint32_t firstArraySlice,
        uint32_t arraySize
    )
        : ResourceView(pDevice, pResource, gfxResourceView, mipLevel, 1, firstArraySlice, arraySize)
    {}

    auto DepthStencilView::create(
        Device* pDevice,
        Texture* pTexture,
        uint32_t mipLevel,
        uint32_t firstArraySlice,
        uint32_t arraySize
    ) -> core::ref<DepthStencilView>
    {
        AP_ASSERT(enum_has_any_flags(pTexture->getBindFlags(), ResourceBindFlags::DepthStencil));
        rhi::ComPtr<rhi::ITextureView> handle{};
        rhi::TextureViewDesc desc = {};
        desc.format = getGFXFormat(pTexture->getFormat());
        desc.aspect = rhi::TextureAspect::All;
        desc.subresourceRange.layer = firstArraySlice;
        desc.subresourceRange.layerCount = arraySize;
        desc.subresourceRange.mip = mipLevel;
        desc.subresourceRange.mipCount = 1;
        auto debugName = pTexture->getName() + "DSV";
        desc.label = debugName.c_str();
        checkResult(pTexture->getGfxTextureResource()->createView(desc, handle.writeRef()), "Failed to create texture DSV");
        return core::ref<DepthStencilView>(new DepthStencilView(pDevice, pTexture, handle, mipLevel, firstArraySlice, arraySize));
    }

    // UnorderedAccessView

    UnorderedAccessView::UnorderedAccessView(
        Device* pDevice,
        Resource* pResource,
        Slang::ComPtr<rhi::ITextureView> gfxResourceView,
        uint32_t mipLevel,
        uint32_t firstArraySlice,
        uint32_t arraySize
    )
        : ResourceView(pDevice, pResource, gfxResourceView, mipLevel, 1, firstArraySlice, arraySize)
    {}

    UnorderedAccessView::UnorderedAccessView(
        Device* pDevice,
        Resource* pResource,
        Slang::ComPtr<rhi::IBuffer> gfxBuffer,
        uint64_t offset,
        uint64_t size
    )
        : ResourceView(pDevice, pResource, gfxBuffer, offset, size)
    {}

    auto UnorderedAccessView::create(
        Device* pDevice,
        Texture* pTexture,
        uint32_t mipLevel,
        uint32_t firstArraySlice,
        uint32_t arraySize
    ) -> core::ref<UnorderedAccessView>
    {
        AP_ASSERT(enum_has_any_flags(pTexture->getBindFlags(), ResourceBindFlags::UnorderedAccess));
        rhi::ComPtr<rhi::ITextureView> handle{};
        rhi::TextureViewDesc desc = {};
        desc.format = getGFXFormat(pTexture->getFormat());
        desc.aspect = rhi::TextureAspect::All;
        desc.subresourceRange.layer = firstArraySlice;
        desc.subresourceRange.layerCount = arraySize;
        desc.subresourceRange.mip = mipLevel;
        desc.subresourceRange.mipCount = 1;
        auto debugName = pTexture->getName() + "UAV";
        desc.label = debugName.c_str();
        checkResult(pTexture->getGfxTextureResource()->createView(desc, handle.writeRef()), "Failed to create texture UAV");
        return core::ref<UnorderedAccessView>(new UnorderedAccessView(pDevice, pTexture, handle, mipLevel, firstArraySlice, arraySize));
    }

    auto UnorderedAccessView::create(Device* pDevice, Buffer* pBuffer, uint64_t offset, uint64_t size) -> core::ref<UnorderedAccessView>
    {
        return core::ref<UnorderedAccessView>(new UnorderedAccessView(pDevice, pBuffer, rhi::ComPtr<rhi::IBuffer>{pBuffer->getGfxBufferResource()}, offset, size));
    }

    // RenderTargetView

    RenderTargetView::RenderTargetView(
        Device* pDevice,
        Resource* pResource,
        Slang::ComPtr<rhi::ITextureView> gfxResourceView,
        uint32_t mipLevel,
        uint32_t firstArraySlice,
        uint32_t arraySize
    )
        : ResourceView(pDevice, pResource, gfxResourceView, mipLevel, 1, firstArraySlice, arraySize)
    {}

    auto RenderTargetView::create(
        Device* pDevice,
        Texture* pTexture,
        uint32_t mipLevel,
        uint32_t firstArraySlice,
        uint32_t arraySize
    ) -> core::ref<RenderTargetView>
    {
        AP_ASSERT(enum_has_any_flags(pTexture->getBindFlags(), ResourceBindFlags::RenderTarget));
        rhi::ComPtr<rhi::ITextureView> handle{};
        rhi::TextureViewDesc desc = {};
        desc.format = getGFXFormat(pTexture->getFormat());
        desc.aspect = rhi::TextureAspect::All;
        desc.subresourceRange.layer = firstArraySlice;
        desc.subresourceRange.layerCount = arraySize;
        desc.subresourceRange.mip = mipLevel;
        desc.subresourceRange.mipCount = 1;
        auto debugName = pTexture->getName() + "RTV";
        desc.label = debugName.c_str();
        checkResult(pTexture->getGfxTextureResource()->createView(desc, handle.writeRef()), "Failed to create texture RTV");
        return core::ref<RenderTargetView>(new RenderTargetView(pDevice, pTexture, handle, mipLevel, firstArraySlice, arraySize));
    }
} // namespace april::graphics