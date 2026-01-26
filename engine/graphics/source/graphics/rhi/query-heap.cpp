// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "query-heap.hpp"
#include "render-device.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>

namespace april::graphics
{
    auto QueryHeap::create(core::ref<Device> p_device, Type type, uint32_t count) -> core::ref<QueryHeap>
    {
        return core::ref<QueryHeap>(new QueryHeap(p_device, type, count));
    }

    QueryHeap::QueryHeap(core::ref<Device> p_device, Type type, uint32_t count) : mp_device(p_device), m_count(count), m_type(type)
    {
        AP_ASSERT(p_device);
        rhi::QueryPoolDesc desc = {};
        desc.count = count;
        switch (type)
        {
        case Type::Timestamp:
            desc.type = rhi::QueryType::Timestamp;
            break;
        default:
            AP_UNREACHABLE();
        }
        checkResult(p_device->getGfxDevice()->createQueryPool(desc, m_gfxQueryPool.writeRef()), "Failed to create query pool");
    }

    auto QueryHeap::reset() -> void
    {
        m_gfxQueryPool->reset();
    }

    auto QueryHeap::breakStrongReferenceToDevice() -> void
    {
        mp_device.breakStrongReference();
    }
} // namespace april::graphics
