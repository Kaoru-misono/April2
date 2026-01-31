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
    // =====================================================
    // TextureView Implementation
    // =====================================================

    TextureView::TextureView(
        Device* pDevice,
        Texture* pTexture,
        Slang::ComPtr<rhi::ITextureView> gfxTextureView,
        ResourceViewDesc const& desc
    )
        : ResourceView(pDevice, desc)
        , mp_texture(pTexture)
        , m_gfxTextureView(std::move(gfxTextureView))
    {}

    auto TextureView::create(Device* pDevice, Texture* pTexture, ResourceViewDesc const& desc) -> core::ref<TextureView>
    {
        AP_ASSERT(pTexture != nullptr, "Texture cannot be null");

        // Validate usage flags based on bind type
        if ((desc.bindFlags & ResourceBindFlags::ShaderResource) != ResourceBindFlags::None)
        {
            AP_ASSERT(enum_has_any_flags(pTexture->getUsage(), TextureUsage::ShaderResource),
                "Texture must have ShaderResource usage for SRV");
        }
        if ((desc.bindFlags & ResourceBindFlags::UnorderedAccess) != ResourceBindFlags::None)
        {
            AP_ASSERT(enum_has_any_flags(pTexture->getUsage(), TextureUsage::UnorderedAccess),
                "Texture must have UnorderedAccess usage for UAV");
        }
        if ((desc.bindFlags & ResourceBindFlags::RenderTarget) != ResourceBindFlags::None)
        {
            AP_ASSERT(enum_has_any_flags(pTexture->getUsage(), TextureUsage::RenderTarget),
                "Texture must have RenderTarget usage for RTV");
        }
        if ((desc.bindFlags & ResourceBindFlags::DepthStencil) != ResourceBindFlags::None)
        {
            AP_ASSERT(enum_has_any_flags(pTexture->getUsage(), TextureUsage::DepthStencil),
                "Texture must have DepthStencil usage for DSV");
        }

        // Build the RHI texture view descriptor
        rhi::TextureViewDesc gfxDesc = {};

        // Determine format
        if (desc.format != ResourceFormat::Unknown)
        {
            gfxDesc.format = getGFXFormat(desc.format);
        }
        else if ((desc.bindFlags & ResourceBindFlags::ShaderResource) != ResourceBindFlags::None)
        {
            // For SRV, convert depth to color format if needed
            gfxDesc.format = getGFXFormat(depthToColorFormat(pTexture->getFormat()));
        }
        else
        {
            gfxDesc.format = getGFXFormat(pTexture->getFormat());
        }

        gfxDesc.aspect = rhi::TextureAspect::All;

        // Set subresource range
        uint32_t mipCount = desc.texture.mipCount;
        uint32_t arraySize = desc.texture.arraySize;
        uint32_t mostDetailedMip = desc.texture.mostDetailedMip;
        uint32_t firstArraySlice = desc.texture.firstArraySlice;

        // Clamp values to texture bounds
        uint32_t resMipCount = pTexture->getMipCount();
        uint32_t resArraySize = pTexture->getArraySize();

        if (mostDetailedMip >= resMipCount) mostDetailedMip = resMipCount - 1;
        if (firstArraySlice >= resArraySize) firstArraySlice = resArraySize - 1;

        if (mipCount == uint32_t(-1)) mipCount = resMipCount - mostDetailedMip;
        else if (mipCount + mostDetailedMip > resMipCount) mipCount = resMipCount - mostDetailedMip;

        if (arraySize == uint32_t(-1)) arraySize = resArraySize - firstArraySlice;
        else if (arraySize + firstArraySlice > resArraySize) arraySize = resArraySize - firstArraySlice;

        gfxDesc.subresourceRange.mip = mostDetailedMip;
        gfxDesc.subresourceRange.mipCount = mipCount;
        gfxDesc.subresourceRange.layer = firstArraySlice;
        gfxDesc.subresourceRange.layerCount = arraySize;

        // Build debug label
        std::string debugName = pTexture->getName();
        if ((desc.bindFlags & ResourceBindFlags::ShaderResource) != ResourceBindFlags::None)
            debugName += "_SRV";
        else if ((desc.bindFlags & ResourceBindFlags::UnorderedAccess) != ResourceBindFlags::None)
            debugName += "_UAV";
        else if ((desc.bindFlags & ResourceBindFlags::RenderTarget) != ResourceBindFlags::None)
            debugName += "_RTV";
        else if ((desc.bindFlags & ResourceBindFlags::DepthStencil) != ResourceBindFlags::None)
            debugName += "_DSV";
        gfxDesc.label = debugName.c_str();

        // Create the view
        rhi::ComPtr<rhi::ITextureView> gfxView;
        checkResult(
            pTexture->getGfxTextureResource()->createView(gfxDesc, gfxView.writeRef()),
            "Failed to create texture view"
        );

        // Create updated desc with clamped values
        ResourceViewDesc finalDesc = desc;
        finalDesc.texture.mostDetailedMip = mostDetailedMip;
        finalDesc.texture.mipCount = mipCount;
        finalDesc.texture.firstArraySlice = firstArraySlice;
        finalDesc.texture.arraySize = arraySize;

        return core::ref<TextureView>(new TextureView(pDevice, pTexture, std::move(gfxView), finalDesc));
    }

    auto TextureView::getResource() const -> Resource*
    {
        return mp_texture;
    }

    // =====================================================
    // BufferView Implementation
    // =====================================================

    BufferView::BufferView(
        Device* pDevice,
        Buffer* pBuffer,
        Slang::ComPtr<rhi::IBuffer> gfxBuffer,
        ResourceViewDesc const& desc
    )
        : ResourceView(pDevice, desc)
        , mp_buffer(pBuffer)
        , m_gfxBuffer(std::move(gfxBuffer))
    {}

    auto BufferView::create(Device* pDevice, Buffer* pBuffer, ResourceViewDesc const& desc) -> core::ref<BufferView>
    {
        AP_ASSERT(pBuffer != nullptr, "Buffer cannot be null");

        // For buffer views, we use the buffer's underlying RHI buffer directly
        // The view is represented by offset/size in the descriptor
        rhi::ComPtr<rhi::IBuffer> gfxBuffer{pBuffer->getGfxBufferResource()};

        // Calculate actual offset and size
        uint64_t offset = desc.buffer.offset;
        uint64_t size = desc.buffer.size;

        if (size == uint64_t(-1))
        {
            size = pBuffer->getSize() - offset;
        }

        AP_ASSERT(offset + size <= pBuffer->getSize(), "Buffer view range exceeds buffer size");

        // Create updated desc with resolved values
        ResourceViewDesc finalDesc = desc;
        finalDesc.dimension = ResourceViewDimension::Buffer;
        finalDesc.buffer.offset = offset;
        finalDesc.buffer.size = size;

        return core::ref<BufferView>(new BufferView(pDevice, pBuffer, std::move(gfxBuffer), finalDesc));
    }

    auto BufferView::getResource() const -> Resource*
    {
        return mp_buffer;
    }

    auto BufferView::getGfxBinding() const -> rhi::Binding
    {
        return rhi::Binding{m_gfxBuffer.get(), rhi::BufferRange{m_desc.buffer.offset, m_desc.buffer.size}};
    }

    // =====================================================
    // Helper Functions for RHI Binding
    // =====================================================

    auto getGfxBindingFromTextureView(TextureView const* view) -> rhi::Binding
    {
        if (!view) return rhi::Binding{};
        return rhi::Binding{view->getGfxTextureView()};
    }

    auto getGfxBindingFromBufferView(BufferView const* view) -> rhi::Binding
    {
        if (!view) return rhi::Binding{};
        auto const& desc = view->getDesc();
        return rhi::Binding{view->getGfxBuffer(), rhi::BufferRange{desc.buffer.offset, desc.buffer.size}};
    }

} // namespace april::graphics
