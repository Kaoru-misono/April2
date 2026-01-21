#include "gpu-profiler.hpp"
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/query-heap.hpp>
#include <graphics/rhi/low-level-context-data.hpp>

#include <core/error/assert.hpp>

namespace april::graphics
{
    auto GpuProfiler::create(core::ref<Device> pDevice) -> core::ref<GpuProfiler>
    {
        return core::ref<GpuProfiler>(new GpuProfiler(pDevice));
    }

    GpuProfiler::GpuProfiler(core::ref<Device> pDevice)
        : mp_device(pDevice)
    {
        m_frames.resize(Device::kInFlightFrameCount);
    }

    GpuProfiler::~GpuProfiler()
    {
        // Release any allocated queries in all frames
        for (auto& frame : m_frames)
        {
            if (frame.queryCount > 0)
            {
                // Note: In a real scenario we'd need to be careful about releasing 
                // while GPU is still using them.
            }
        }
    }

    auto GpuProfiler::beginEvent(CommandContext* pContext, char const* name) -> void
    {
        if (!pContext) return;
        auto pDevice = mp_device.get();
        auto pQueryHeap = pDevice->getTimestampQueryHeap();

        auto& frame = m_frames[m_currentFrameIndex];
        
        auto startIndex = pQueryHeap->allocate();
        if (startIndex == QueryHeap::kInvalidIndex) return;

        pContext->writeTimestamp(pQueryHeap.get(), startIndex);

        auto event = Event{
            .name = name ? name : "",
            .startIndex = startIndex,
            .endIndex = QueryHeap::kInvalidIndex,
            .level = static_cast<uint32_t>(m_eventStack.size())
        };

        m_eventStack.push_back(static_cast<uint32_t>(frame.events.size()));
        frame.events.push_back(event);
        frame.queryCount++;
    }

    auto GpuProfiler::endEvent(CommandContext* pContext) -> void
    {
        if (m_eventStack.empty() || !pContext) return;

        auto pDevice = mp_device.get();
        auto pQueryHeap = pDevice->getTimestampQueryHeap();

        auto& frame = m_frames[m_currentFrameIndex];
        auto eventIndex = m_eventStack.back();
        m_eventStack.pop_back();

        auto endIndex = pQueryHeap->allocate();
        if (endIndex == QueryHeap::kInvalidIndex) return;

        pContext->writeTimestamp(pQueryHeap.get(), endIndex);
        frame.events[eventIndex].endIndex = endIndex;
        frame.queryCount++;
    }

    // ... resolve and update implementation ...

    // --- ScopedGpuProfileEvent ---

    ScopedGpuProfileEvent::ScopedGpuProfileEvent(CommandContext* pContext, char const* name)
        : m_context(pContext)
    {
        if (m_context)
        {
            auto pDevice = m_context->getDevice();
            if (pDevice)
            {
                m_profiler = pDevice->getGpuProfiler();
                if (m_profiler)
                {
                    m_profiler->beginEvent(m_context, name);
                }
            }
        }
    }

    ScopedGpuProfileEvent::~ScopedGpuProfileEvent()
    {
        if (m_profiler && m_context)
        {
            m_profiler->endEvent(m_context);
        }
    }

} // namespace april::graphics
