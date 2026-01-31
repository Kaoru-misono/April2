// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "texture.hpp"
#include "render-device.hpp"
#include "command-context.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>
#include <core/log/logger.hpp>
#include <core/math/math.hpp>
#include <core/tools/enum-flags.hpp>
#include <core/tools/alignment.hpp>

#include <bit>
#include <mutex>

namespace april::graphics
{
    inline namespace
    {
        auto getGfxTextureType(Resource::Type type) -> rhi::TextureType
        {
            switch (type)
            {
            case Resource::Type::Texture1D:
                return rhi::TextureType::Texture1D;
            case Resource::Type::Texture2D:
                return rhi::TextureType::Texture2D;
            case Resource::Type::Texture2DMS:
                return rhi::TextureType::Texture2DMS;
            case Resource::Type::TextureCube:
                return rhi::TextureType::TextureCube;
            case Resource::Type::Texture3D:
                return rhi::TextureType::Texture3D;
            default:
                AP_UNREACHABLE();
            }
        }

        auto getTextureUsage(graphics::TextureUsage flags) -> rhi::TextureUsage
        {
            rhi::TextureUsage usage = rhi::TextureUsage::None;

            // 1. Shader Resource (SRV)
            if (enum_has_any_flags(flags, graphics::TextureUsage::ShaderResource))
            {
                usage |= rhi::TextureUsage::ShaderResource;
            }

            // 2. Unordered Access (UAV)
            if (enum_has_any_flags(flags, graphics::TextureUsage::UnorderedAccess))
            {
                usage |= rhi::TextureUsage::UnorderedAccess;
            }

            // 3. Render Target (RTV)
            if (enum_has_any_flags(flags, graphics::TextureUsage::RenderTarget))
            {
                usage |= rhi::TextureUsage::RenderTarget;
            }

            // 4. Depth Stencil (DSV)
            if (enum_has_any_flags(flags, graphics::TextureUsage::DepthStencil))
            {
                usage |= rhi::TextureUsage::DepthStencil;
            }

            // 5. Shared
            if (enum_has_any_flags(flags, graphics::TextureUsage::Shared))
            {
                usage |= rhi::TextureUsage::Shared;
            }

            usage |= rhi::TextureUsage::CopySource | rhi::TextureUsage::CopyDestination;

            return usage;
        }

        auto bitScanReverse(uint32_t v) -> uint32_t
        {
            if (v == 0) return 0;
            return (uint32_t)std::bit_width(v) - 1;
        }
    } // namespace

    Texture::Texture(
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
    )
        : Resource(p_device, type, 0)
        , m_usage(usage)
        , m_format(format)
        , m_width(width)
        , m_height(height)
        , m_depth(depth)
        , m_mipLevels(mipLevels)
        , m_arraySize(arraySize)
        , m_sampleCount(sampleCount)
    {
        AP_ASSERT(m_type != Type::Buffer, "Texture type cannot be Buffer.");
        AP_ASSERT(m_format != ResourceFormat::Unknown, "Texture format cannot be Unknown.");
        AP_ASSERT(m_width > 0 && m_height > 0 && m_depth > 0, "Texture dimensions must be greater than zero.");

        bool autoGenerateMips = pInitData && (m_mipLevels == kMaxPossible);

        if (autoGenerateMips)
        {
            m_usage |= TextureUsage::RenderTarget;
        }

        if (m_mipLevels == kMaxPossible)
        {
            uint32_t dims = m_width | m_height | m_depth;
            m_mipLevels = bitScanReverse(dims) + 1;
        }

        // Default initial state.
        if (enum_has_any_flags(m_usage, TextureUsage::RenderTarget)) m_state.global = Resource::State::RenderTarget;
        else if (enum_has_any_flags(m_usage, TextureUsage::DepthStencil)) m_state.global = Resource::State::DepthStencil;
        else if (enum_has_any_flags(m_usage, TextureUsage::UnorderedAccess)) m_state.global = Resource::State::UnorderedAccess;
        else if (enum_has_any_flags(m_usage, TextureUsage::ShaderResource)) m_state.global = Resource::State::ShaderResource;

        m_state.perSubresource.resize(m_mipLevels * m_arraySize, m_state.global);

        rhi::TextureDesc desc = {};
        desc.type = getGfxTextureType(m_type);
        desc.usage = getTextureUsage(m_usage);
        desc.defaultState = rhi::ResourceState::General;
        desc.memoryType = rhi::MemoryType::DeviceLocal;

        desc.size.width = align_up(m_width, getFormatWidthCompressionRatio(m_format));
        desc.size.height = align_up(m_height, getFormatHeightCompressionRatio(m_format));
        desc.size.depth = m_depth;

        desc.arrayLength = m_type == Resource::Type::TextureCube ? m_arraySize * 6 : m_arraySize;
        desc.mipCount = m_mipLevels;
        desc.format = getGFXFormat(m_format);
        desc.sampleCount = m_sampleCount;
        desc.sampleQuality = 0;

        if (enum_has_any_flags(m_usage, TextureUsage::RenderTarget | TextureUsage::DepthStencil))
        {
            rhi::ClearValue clearValue = {};
            if (enum_has_any_flags(m_usage, TextureUsage::DepthStencil))
            {
                clearValue.depthStencil.depth = 1.0f;
            }
            desc.optimalClearValue = &clearValue;
        }

        if (enum_has_any_flags(m_usage, TextureUsage::Shared))
        {
            desc.usage |= rhi::TextureUsage::Shared;
        }

        {
            std::lock_guard<std::mutex> lock(mp_device->getGlobalGfxMutex());

            rhi::SubresourceData initData = {};
            rhi::SubresourceData* pDataPtr = nullptr;
            if (pInitData)
            {
                initData.data = pInitData;
                initData.rowPitch = (uint64_t)m_width * getFormatBytesPerBlock(m_format);
                initData.slicePitch = initData.rowPitch * m_height;
                pDataPtr = &initData;
            }

            checkResult(mp_device->getGfxDevice()->createTexture(desc, pDataPtr, m_gfxTexture.writeRef()), "Failed to create texture resource");
            AP_ASSERT(m_gfxTexture);

            // TODO:
            // if (pInitData)
            // {
            //     incRef();
            //     uploadInitData(mp_device->getCommandContext(), pInitData, autoGenerateMips);
            //     decRef(false);
            // }
        }
    }

    Texture::Texture(
        core::ref<Device> pDevice,
        rhi::ITexture* pTexture,
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
    )
        : Resource(pDevice, type, 0), m_usage(usage), m_format(format), m_width(width), m_height(height), m_depth(depth), m_mipLevels(mipLevels), m_arraySize(arraySize), m_sampleCount(sampleCount)
    {
        m_gfxTexture = pTexture;
        m_state.global = initState;
        m_state.isGlobal = true;
    }

    Texture::~Texture()
    {
        mp_device->releaseResource(m_gfxTexture.get());
    }

    auto Texture::getGfxResource() const -> rhi::IResource*
    {
        return m_gfxTexture.get();
    }

    auto Texture::invalidateViews() const -> void
    {
        m_srvs.clear();
        m_rtvs.clear();
        m_dsvs.clear();
        m_uavs.clear();
    }

    auto Texture::getSRV(uint32_t mostDetailedMip, uint32_t mipCount, uint32_t firstArraySlice, uint32_t arraySize) -> core::ref<TextureView>
    {
        uint32_t resMipCount = getMipCount();
        uint32_t resArraySize = getArraySize();

        if (firstArraySlice >= resArraySize) firstArraySlice = resArraySize - 1;
        if (mostDetailedMip >= resMipCount) mostDetailedMip = resMipCount - 1;

        if (mipCount == kMaxPossible) mipCount = resMipCount - mostDetailedMip;
        else if (mipCount + mostDetailedMip > resMipCount) mipCount = resMipCount - mostDetailedMip;

        if (arraySize == kMaxPossible) arraySize = resArraySize - firstArraySlice;
        else if (arraySize + firstArraySlice > resArraySize) arraySize = resArraySize - firstArraySlice;

        ResourceViewInfo viewInfo = ResourceViewInfo(mostDetailedMip, mipCount, firstArraySlice, arraySize);

        if (m_srvs.find(viewInfo) == m_srvs.end())
        {
            ResourceViewDesc desc;
            desc.bindFlags = ResourceBindFlags::ShaderResource;
            desc.texture.mostDetailedMip = mostDetailedMip;
            desc.texture.mipCount = mipCount;
            desc.texture.firstArraySlice = firstArraySlice;
            desc.texture.arraySize = arraySize;
            m_srvs[viewInfo] = TextureView::create(mp_device.get(), this, desc);
        }

        return m_srvs[viewInfo];
    }

    auto Texture::getRTV(uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize) -> core::ref<TextureView>
    {
        uint32_t resMipCount = getMipCount();
        uint32_t resArraySize = getArraySize();

        if (firstArraySlice >= resArraySize) firstArraySlice = resArraySize - 1;
        if (mipLevel >= resMipCount) mipLevel = resMipCount - 1;

        if (arraySize == kMaxPossible) arraySize = resArraySize - firstArraySlice;
        else if (arraySize + firstArraySlice > resArraySize) arraySize = resArraySize - firstArraySlice;

        ResourceViewInfo viewInfo = ResourceViewInfo(mipLevel, 1, firstArraySlice, arraySize);

        if (m_rtvs.find(viewInfo) == m_rtvs.end())
        {
            ResourceViewDesc desc;
            desc.bindFlags = ResourceBindFlags::RenderTarget;
            desc.texture.mostDetailedMip = mipLevel;
            desc.texture.mipCount = 1;
            desc.texture.firstArraySlice = firstArraySlice;
            desc.texture.arraySize = arraySize;
            m_rtvs[viewInfo] = TextureView::create(mp_device.get(), this, desc);
        }

        return m_rtvs[viewInfo];
    }

    auto Texture::getDSV(uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize) -> core::ref<TextureView>
    {
        uint32_t resMipCount = getMipCount();
        uint32_t resArraySize = getArraySize();

        if (firstArraySlice >= resArraySize) firstArraySlice = resArraySize - 1;
        if (mipLevel >= resMipCount) mipLevel = resMipCount - 1;

        if (arraySize == kMaxPossible) arraySize = resArraySize - firstArraySlice;
        else if (arraySize + firstArraySlice > resArraySize) arraySize = resArraySize - firstArraySlice;

        ResourceViewInfo viewInfo = ResourceViewInfo(mipLevel, 1, firstArraySlice, arraySize);

        if (m_dsvs.find(viewInfo) == m_dsvs.end())
        {
            ResourceViewDesc desc;
            desc.bindFlags = ResourceBindFlags::DepthStencil;
            desc.texture.mostDetailedMip = mipLevel;
            desc.texture.mipCount = 1;
            desc.texture.firstArraySlice = firstArraySlice;
            desc.texture.arraySize = arraySize;
            m_dsvs[viewInfo] = TextureView::create(mp_device.get(), this, desc);
        }

        return m_dsvs[viewInfo];
    }

    auto Texture::getUAV(uint32_t mipLevel, uint32_t firstArraySlice, uint32_t arraySize) -> core::ref<TextureView>
    {
        uint32_t resMipCount = getMipCount();
        uint32_t resArraySize = getArraySize();

        if (firstArraySlice >= resArraySize) firstArraySlice = resArraySize - 1;
        if (mipLevel >= resMipCount) mipLevel = resMipCount - 1;

        if (arraySize == kMaxPossible) arraySize = resArraySize - firstArraySlice;
        else if (arraySize + firstArraySlice > resArraySize) arraySize = resArraySize - firstArraySlice;

        ResourceViewInfo viewInfo = ResourceViewInfo(mipLevel, 1, firstArraySlice, arraySize);

        if (m_uavs.find(viewInfo) == m_uavs.end())
        {
            ResourceViewDesc desc;
            desc.bindFlags = ResourceBindFlags::UnorderedAccess;
            desc.texture.mostDetailedMip = mipLevel;
            desc.texture.mipCount = 1;
            desc.texture.firstArraySlice = firstArraySlice;
            desc.texture.arraySize = arraySize;
            m_uavs[viewInfo] = TextureView::create(mp_device.get(), this, desc);
        }

        return m_uavs[viewInfo];
    }

    auto Texture::getSubresourceLayout(uint32_t subresource) const -> SubresourceLayout
    {
        AP_ASSERT(subresource < getSubresourceCount());

        rhi::ITexture* gfxTexture = getGfxTextureResource();
        rhi::FormatInfo gfxFormatInfo = rhi::getFormatInfo(gfxTexture->getDesc().format);

        SubresourceLayout layout = {};
        uint32_t mipLevel = getSubresourceMipLevel(subresource);
        layout.rowSize = align_up(getWidth(mipLevel), (uint32_t)gfxFormatInfo.blockWidth) * gfxFormatInfo.blockSizeInBytes;
        layout.rowSizeAligned = align_up(layout.rowSize, (size_t)mp_device->getTextureRowAlignment());
        layout.rowCount = align_up(getHeight(mipLevel), (uint32_t)gfxFormatInfo.blockHeight);
        layout.depth = getDepth(mipLevel);

        return layout;
    }

    auto Texture::setSubresourceBlob(uint32_t subresource, void const* pData, size_t size) -> void
    {
        AP_ASSERT(subresource < getSubresourceCount());
        SubresourceLayout layout = getSubresourceLayout(subresource);
        AP_ASSERT(size == layout.getTotalByteSize());

        // TODO:
        // mp_device->getCommandContext()->updateSubresourceData(this, subresource, pData);
    }

    auto Texture::getSubresourceBlob(uint32_t subresource, void* pData, size_t size) const -> void
    {
        AP_ASSERT(subresource < getSubresourceCount());
        SubresourceLayout layout = getSubresourceLayout(subresource);
        AP_ASSERT(size == layout.getTotalByteSize());

        // TODO:
        // auto data = mp_device->getCommandContext()->readTextureSubresource(this, subresource);
        // AP_ASSERT(data.size() == size);
        // std::memcpy(pData, data.data(), data.size());
    }

    auto Texture::uploadInitData(CommandContext* pCommandContext, void const* pData, bool autoGenMips) -> void
    {
        // TODO:
        if (autoGenMips)
        {
            size_t arraySliceSize = m_width * m_height * getFormatBytesPerBlock(m_format);
            uint8_t const* pSrc = (uint8_t const*)pData;
            uint32_t numFaces = (m_type == Type::TextureCube) ? 6 : 1;
            for (uint32_t i = 0; i < m_arraySize * numFaces; i++)
            {
                uint32_t subresource = getSubresourceIndex(i, 0);
                // pCommandContext->updateSubresourceData(this, subresource, pSrc);
                pSrc += arraySliceSize;
            }
        }
        else
        {
            // pCommandContext->updateTextureData(this, pData);
        }

        if (autoGenMips)
        {
            generateMips(pCommandContext);
            invalidateViews();
        }
    }

    auto Texture::generateMips(CommandContext* pContext, bool minMaxMips) -> void
    {
        for (uint32_t m = 0; m < m_mipLevels - 1; m++)
        {
            for (uint32_t a = 0; a < m_arraySize; a++)
            {
                auto srv = getSRV(m, 1, a, 1);
                auto rtv = getRTV(m + 1, a, 1);
                // TODO:
                // pContext->blit(srv, rtv);
            }
        }

        if (m_releaseRtvsAfterGenMips)
        {
            m_rtvs.clear();
            m_releaseRtvsAfterGenMips = false;
        }
    }

    auto Texture::getTexelCount() const -> uint64_t
    {
        uint64_t count = 0;
        for (uint32_t i = 0; i < getMipCount(); i++)
        {
            count += (uint64_t)getWidth(i) * getHeight(i) * getDepth(i);
        }
        count *= getArraySize();
        return count;
    }

    auto Texture::compareDesc(Texture const* pOther) const -> bool
    {
        return m_width == pOther->m_width && m_height == pOther->m_height && m_depth == pOther->m_depth && m_mipLevels == pOther->m_mipLevels &&
               m_sampleCount == pOther->m_sampleCount && m_arraySize == pOther->m_arraySize && m_format == pOther->m_format;
    }

    auto Texture::getTextureSizeInBytes() const -> uint64_t
    {
        size_t outSizeBytes = 0, outAlignment = 0;
        auto& desc = m_gfxTexture->getDesc();
        mp_device->getGfxDevice()->getTextureAllocationInfo(desc, &outSizeBytes, &outAlignment);
        return outSizeBytes;
    }

} // namespace april::graphics
