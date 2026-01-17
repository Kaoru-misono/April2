// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "gpu-timer.hpp"
#include "render-device.hpp"
#include "command-context.hpp"
#include "query-heap.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>
#include <core/log/logger.hpp>

namespace april::graphics
{
    auto GpuTimer::create(core::ref<Device> device) -> core::ref<GpuTimer>
    {
        return core::ref<GpuTimer>(new GpuTimer(device));
    }

    GpuTimer::GpuTimer(core::ref<Device> device) : mp_device(device)
    {
        AP_ASSERT(mp_device);

        mp_resolveBuffer = mp_device->createBuffer(sizeof(uint64_t) * 2, ResourceBindFlags::None, MemoryType::DeviceLocal, nullptr);
        mp_resolveBuffer->breakStrongReferenceToDevice();
        mp_resolveStagingBuffer = mp_device->createBuffer(sizeof(uint64_t) * 2, ResourceBindFlags::None, MemoryType::ReadBack, nullptr);
        mp_resolveStagingBuffer->breakStrongReferenceToDevice();

        // Create timestamp query heap upon first use.
        m_start = mp_device->getTimestampQueryHeap()->allocate();
        m_end = mp_device->getTimestampQueryHeap()->allocate();
        if (m_start == QueryHeap::kInvalidIndex || m_end == QueryHeap::kInvalidIndex)
        {
            AP_ERROR("Can't create GPU timer, no available timestamp queries.");
        }
        AP_ASSERT(m_end == (m_start + 1));
    }

    GpuTimer::~GpuTimer()
    {
        mp_device->getTimestampQueryHeap()->release(m_start);
        mp_device->getTimestampQueryHeap()->release(m_end);
    }

    auto GpuTimer::begin() -> void
    {
        if (m_status == Status::Begin)
        {
            AP_WARN("GpuTimer::begin() was followed by another call to GpuTimer::begin() without a GpuTimer::end() in-between. Ignoring call.");
            return;
        }

        if (m_status == Status::End)
        {
            AP_WARN("GpuTimer::begin() was followed by a call to GpuTimer::end() without querying the data first. The previous results will be discarded.");
        }

        mp_device->getCommandContext()->getLowLevelData()->getGfxCommandEncoder()->writeTimestamp(
            mp_device->getTimestampQueryHeap()->getGfxQueryPool(), m_start
        );
        m_status = Status::Begin;
    }

    auto GpuTimer::end() -> void
    {
        if (m_status != Status::Begin)
        {
            AP_WARN("GpuTimer::end() was called without a preceding GpuTimer::begin(). Ignoring call.");
            return;
        }

        mp_device->getCommandContext()->getLowLevelData()->getGfxCommandEncoder()->writeTimestamp(
            mp_device->getTimestampQueryHeap()->getGfxQueryPool(), m_end
        );
        m_status = Status::End;
    }

    auto GpuTimer::resolve() -> void
    {
        if (m_status == Status::Idle)
        {
            return;
        }

        if (m_status == Status::Begin)
        {
            AP_ERROR("GpuTimer::resolve() was called but the GpuTimer::end() wasn't called.");
            return;
        }

        AP_ASSERT(m_status == Status::End, "GpuTimer must be in End state before resolve.");

        // TODO: The code here is inefficient as it resolves each timer individually.
        // This should be batched across all active timers and results copied into a single staging buffer once per frame instead.

        auto encoder = mp_device->getCommandContext()->getLowLevelData()->getGfxCommandEncoder();

        encoder->resolveQuery(mp_device->getTimestampQueryHeap()->getGfxQueryPool(), m_start, 2, mp_resolveBuffer->getGfxBufferResource(), 0);

        mp_device->getCommandContext()->copyBuffer(mp_resolveStagingBuffer.get(), mp_resolveBuffer.get());

        m_dataPending = true;
        m_status = Status::Idle;
    }

    auto GpuTimer::getElapsedTime() -> double
    {
        if (m_status == Status::Begin)
        {
            AP_WARN("GpuTimer::getElapsedTime() was called but the GpuTimer::end() wasn't called. No data to fetch.");
            return 0.0;
        }
        else if (m_status == Status::End)
        {
            AP_WARN("GpuTimer::getElapsedTime() was called but the GpuTimer::resolve() wasn't called. No data to fetch.");
            return 0.0;
        }

        AP_ASSERT(m_status == Status::Idle);
        if (m_dataPending)
        {
            uint64_t result[2];
            uint64_t* pRes = (uint64_t*)mp_resolveStagingBuffer->map();
            result[0] = pRes[0];
            result[1] = pRes[1];
            mp_resolveStagingBuffer->unmap();

            double start = (double)result[0];
            double end = (double)result[1];
            double range = end - start;
            m_elapsedTime = range * mp_device->getGpuTimestampFrequency();
            m_dataPending = false;
        }
        return m_elapsedTime;
    }

    auto GpuTimer::breakStrongReferenceToDevice() -> void
    {
        mp_device.breakStrongReference();
    }
} // namespace april::graphics
