// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "buffer.hpp"
#include "render-device.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>
#include <core/math/math.hpp>
#include <core/tools/enum-flags.hpp>
#include <core/tools/alignment.hpp>

namespace april::graphics
{
    inline namespace
    {
        auto gfxResourceFromNativeHandle(
            Device* p_device,
            NativeHandle handle,
            size_t size,
            BufferUsage usage,
            MemoryType memoryType
        ) -> Slang::ComPtr<rhi::IBuffer>
        {
            rhi::BufferDesc bufDesc = {};
            prepareGFXBufferDesc(bufDesc, Resource::State::Undefined, size, 0, ResourceFormat::Unknown, usage, memoryType);

//             rhi::InteropHandle gfxNativeHandle = {};
//             // Assuming handle contains the raw pointer/handle based on platform
//             // TODO:
// //             gfxNativeHandle.handleValue = (uint64_t)handle;
// // #if defined(_WIN32)
// //             gfxNativeHandle.api = rhi::InteropHandleAPI::D3D12;
// // #else
// //             gfxNativeHandle.api = rhi::InteropHandleAPI::Vulkan;
// // #endif
//
//             if (gfxNativeHandle.api == rhi::InteropHandleAPI::Unknown) AP_CRITICAL("Unknown native handle type");
//
//             Slang::ComPtr<rhi::IBufferResource> gfxBuffer;
//             checkResult(p_device->getGfxDevice()->createBufferFromNativeHandle(gfxNativeHandle, bufDesc, gfxBuffer.writeRef()), "Failed to create buffer from native handle");
//
//             return gfxBuffer;
            return nullptr;
        }
    }

    Buffer::Buffer(
        core::ref<Device> p_device,
        size_t size,
        size_t structSize,
        ResourceFormat format,
        BufferUsage usage,
        MemoryType memoryType,
        void const* pInitData
    )
        : Resource(p_device, Type::Buffer, size), m_memoryType(memoryType), m_usage(usage)
    {
        AP_ASSERT(size > 0, "Can't create GPU buffer of size zero");

        // Check that buffer size is within 4GB limit. Larger buffers are currently not well supported in D3D12.
        // TODO: Revisit this check in the future.
        AP_ASSERT(size <= (1ull << 32), "Creating GPU buffer of size {} bytes. Buffers above 4GB are not currently well supported.", size);

        if (m_memoryType != MemoryType::DeviceLocal && enum_has_any_flags(m_usage, BufferUsage::Shared))
        {
            AP_ERROR("Can't create shared resource with CPU access other than 'None'.");
        }

        // Align size if needed
        m_size = align_up(m_size, mp_device->getBufferDataAlignment(m_usage));
        m_structSize = (uint32_t)structSize;
        m_format = format;

        if (m_memoryType == MemoryType::DeviceLocal)
        {
            m_state.global = Resource::State::Common;
            if (enum_has_any_flags(m_usage, BufferUsage::AccelerationStructure))
                 m_state.global = Resource::State::AccelerationStructureRead | Resource::State::AccelerationStructureWrite;
        }
        else if (m_memoryType == MemoryType::Upload)
        {
            m_state.global = Resource::State::GenericRead;
        }
        else if (m_memoryType == MemoryType::ReadBack)
        {
            m_state.global = Resource::State::CopyDest;
        }

        m_gfxBuffer = createBufferResource(mp_device, m_state.global, m_size, m_structSize, m_format, m_usage, m_memoryType);

        if (pInitData)
            setBlob(pInitData, 0, size);

        m_elementCount = (uint32_t)size;
    }

    Buffer::Buffer(core::ref<Device> p_device, size_t size, BufferUsage usage, MemoryType memoryType, void const* pInitData)
        : Buffer(p_device, size, 0, ResourceFormat::Unknown, usage, memoryType, pInitData)
    {}

    Buffer::Buffer(
        core::ref<Device> p_device,
        ResourceFormat format,
        uint32_t elementCount,
        BufferUsage usage,
        MemoryType memoryType,
        void const* pInitData
    )
        : Buffer(p_device, (size_t)getFormatBytesPerBlock(format) * elementCount, 0, format, usage, memoryType, pInitData)
    {
        m_elementCount = elementCount;
    }

    Buffer::Buffer(
        core::ref<Device> p_device,
        uint32_t structSize,
        uint32_t elementCount,
        BufferUsage usage,
        MemoryType memoryType,
        void const* pInitData,
        bool createCounter
    )
        : Buffer(p_device, (size_t)structSize * elementCount, structSize, ResourceFormat::Unknown, usage, memoryType, pInitData)
    {
        m_elementCount = elementCount;
        static const uint32_t zero = 0;
        if (createCounter)
        {
            AP_ASSERT(m_structSize > 0, "Can't create a counter buffer with struct size of 0.");
            m_uavCounter = core::make_ref<Buffer>(
                mp_device,
                sizeof(uint32_t),
                sizeof(uint32_t),
                ResourceFormat::Unknown,
                BufferUsage::UnorderedAccess,
                MemoryType::DeviceLocal,
                &zero
            );
        }
    }

    // TODO: Its wasteful to create a buffer just to replace it afterwards with the supplied one!
    Buffer::Buffer(core::ref<Device> p_device, rhi::IBuffer* pResource, size_t size, BufferUsage usage, MemoryType memoryType)
        : Buffer(p_device, size, 0, ResourceFormat::Unknown, usage, memoryType, nullptr)
    {
        AP_ASSERT(pResource);
        m_gfxBuffer = pResource;
    }

    Buffer::Buffer(core::ref<Device> p_device, NativeHandle handle, size_t size, BufferUsage usage, MemoryType memoryType)
        : Buffer(p_device, gfxResourceFromNativeHandle(p_device.get(), handle, size, usage, memoryType), size, usage, memoryType)
    {}

    Buffer::~Buffer()
    {
        unmap();
        mp_device->releaseResource(m_gfxBuffer);
    }

    auto Buffer::getGfxResource() const -> rhi::IResource*
    {
        return m_gfxBuffer;
    }

    auto Buffer::getSRV(uint64_t offset, uint64_t size) -> core::ref<ShaderResourceView>
    {
        ResourceViewInfo view = ResourceViewInfo(offset, size);

        if (m_srvs.find(view) == m_srvs.end())
            m_srvs[view] = ShaderResourceView::create(getDevice().get(), this, offset, size);

        return m_srvs[view];
    }

    auto Buffer::getSRV() -> core::ref<ShaderResourceView>
    {
        return getSRV(0);
    }

    auto Buffer::getUAV(uint64_t offset, uint64_t size) -> core::ref<UnorderedAccessView>
    {
        ResourceViewInfo view = ResourceViewInfo(offset, size);

        if (m_uavs.find(view) == m_uavs.end())
            m_uavs[view] = UnorderedAccessView::create(getDevice().get(), this, offset, size);

        return m_uavs[view];
    }

    auto Buffer::getUAV() -> core::ref<UnorderedAccessView>
    {
        return getUAV(0);
    }

    auto Buffer::setBlob(void const* pData, size_t offset, size_t size) -> void
    {
        AP_ASSERT(offset + size <= m_size, "'offset' ({}) and 'size' ({}) don't fit the buffer size {}.", offset, size, m_size);

        if (m_memoryType == MemoryType::Upload)
        {
            bool wasMapped = m_mappedPtr != nullptr;
            uint8_t* pDst = (uint8_t*)map(rhi::CpuAccessMode::Write) + offset;
            std::memcpy(pDst, pData, size);
            if (!wasMapped)
                unmap();
            // TODO we should probably use a barrier instead
            invalidateViews();
        }
        else if (m_memoryType == MemoryType::DeviceLocal)
        {
            // TODO:
            // mp_device->getRenderContext()->updateBuffer(this, pData, offset, size);
        }
        else if (m_memoryType == MemoryType::ReadBack)
        {
            AP_ERROR("Cannot set data to a buffer that was created with MemoryType::ReadBack.");
        }
    }

    auto Buffer::getBlob(void* pData, size_t offset, size_t size) const -> void
    {
        AP_ASSERT(offset + size <= m_size, "'offset' ({}) and 'size' ({}) don't fit the buffer size {}.", offset, size, m_size);

        if (m_memoryType == MemoryType::ReadBack)
        {
            bool wasMapped = m_mappedPtr != nullptr;
            uint8_t const* pSrc = (uint8_t const*)map(rhi::CpuAccessMode::Read) + offset;
            std::memcpy(pData, pSrc, size);
            if (!wasMapped)
                unmap();
        }
        else if (m_memoryType == MemoryType::DeviceLocal)
        {
            // TODO:
            // mp_device->getRenderContext()->readBuffer(this, pData, offset, size);
        }
        else if (m_memoryType == MemoryType::Upload)
        {
            AP_ERROR("Cannot get data from a buffer that was created with MemoryType::Upload.");
        }
    }

    auto Buffer::map(rhi::CpuAccessMode mode) const -> void*
    {
        if (m_mappedPtr == nullptr)
        {
            checkResult(mp_device->getGfxDevice()->mapBuffer(m_gfxBuffer, mode, &m_mappedPtr), "Failed to map buffer");
        }
        return m_mappedPtr;
    }

    auto Buffer::unmap() const -> void
    {
        if (m_mappedPtr)
        {
            mp_device->getGfxDevice()->unmapBuffer(m_gfxBuffer);
            m_mappedPtr = nullptr;
        }
    }

    auto Buffer::adjustSizeOffsetParams(size_t& size, size_t& offset) const -> bool
    {
        if (offset >= m_size)
        {
            AP_ERROR("Buffer::adjustSizeOffsetParams() - offset is larger than the buffer size.");
            return false;
        }

        if (offset + size > m_size)
        {
            AP_ERROR("Buffer::adjustSizeOffsetParams() - offset + size will cause an OOB access. Clamping size");
            size = m_size - offset;
        }
        return true;
    }

    auto Buffer::getGpuAddress() const -> uint64_t
    {
        return m_gfxBuffer->getDeviceAddress();
    }
} // namespace april::graphics
