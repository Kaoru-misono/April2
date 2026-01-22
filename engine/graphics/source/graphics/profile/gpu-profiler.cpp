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

    auto GpuProfiler::beginAsyncEvent(CommandContext* pContext, char const* name) -> uint32_t
    {
        if (!pContext) return uint32_t(-1);
        auto pDevice = mp_device.get();
        auto pQueryHeap = pDevice->getTimestampQueryHeap();

        auto startIndex = pQueryHeap->allocate();
        if (startIndex == QueryHeap::kInvalidIndex) return uint32_t(-1);

        pContext->writeTimestamp(pQueryHeap.get(), startIndex);

        auto event = AsyncEvent{};
        event.name = name ? name : "";
        event.startIndex = startIndex;
        event.endIndex = QueryHeap::kInvalidIndex;
        event.level = 0; // Async events are flat for now
        event.isFinished = false;

        uint32_t id = static_cast<uint32_t>(m_asyncEvents.size());
        m_asyncEvents.push_back(event);
        return id;
    }

    auto GpuProfiler::endAsyncEvent(CommandContext* pContext, uint32_t asyncId) -> void
    {
        if (asyncId >= m_asyncEvents.size() || !pContext) return;

        auto pDevice = mp_device.get();
        auto pQueryHeap = pDevice->getTimestampQueryHeap();

        auto endIndex = pQueryHeap->allocate();
        if (endIndex == QueryHeap::kInvalidIndex) return;

        pContext->writeTimestamp(pQueryHeap.get(), endIndex);
        m_asyncEvents[asyncId].endIndex = endIndex;
        m_asyncEvents[asyncId].isFinished = true;
    }

    auto GpuProfiler::resolve(CommandContext* pContext) -> void
    {
        if (!pContext) return;
        auto& frame = m_frames[m_currentFrameIndex];
        if (frame.queryCount == 0) return;

        auto pDevice = mp_device.get();
        auto pQueryHeap = pDevice->getTimestampQueryHeap();

        // Create staging buffer if not exists
        if (!frame.pResolveBuffer)
        {
            frame.pResolveBuffer = pDevice->createBuffer(
                pQueryHeap->getQueryCount() * sizeof(uint64_t),
                ResourceBindFlags::None,
                MemoryType::ReadBack
            );
        }

        pContext->resolveQuery(
            pQueryHeap.get(),
            0,
            pQueryHeap->getQueryCount(),
            frame.pResolveBuffer.get(),
            0
        );

        frame.isResolved = true;

        // Advance frame index
        m_currentFrameIndex = (m_currentFrameIndex + 1) % Device::kInFlightFrameCount;
        
        // Prepare next frame (reset it)
        auto& nextFrame = m_frames[m_currentFrameIndex];
        // We can only reset if it's already updated (meaning GPU is done)
        // In a real engine, we'd wait on a fence.
        nextFrame.events.clear();
        nextFrame.queryCount = 0;
        nextFrame.isResolved = false;
    }

    auto GpuProfiler::update() -> void
    {
        auto pDevice = mp_device.get();
        if (!pDevice) return;

        // We update the frame that was just resolved (or a few frames ago)
        // For simplicity, let's try to update the frame before the current one.
        uint32_t frameIdx = (m_currentFrameIndex + Device::kInFlightFrameCount - 1) % Device::kInFlightFrameCount;
        auto& frame = m_frames[frameIdx];

        if (!frame.isResolved || frame.events.empty()) return;

        // Map buffer and read results
        auto pData = static_cast<uint64_t const*>(frame.pResolveBuffer->map());
        if (!pData) return;

        double frequency = pDevice->getGpuTimestampFrequency(); // ms/tick

        for (auto const& event : frame.events)
        {
            if (event.startIndex != QueryHeap::kInvalidIndex && event.endIndex != QueryHeap::kInvalidIndex)
            {
                uint64_t start = pData[event.startIndex];
                uint64_t end = pData[event.endIndex];

                if (end >= start)
                {
                    double durationTicks = static_cast<double>(end - start);
                    double durationMs = durationTicks * frequency;
                    double durationUs = durationMs * 1000.0;

                    core::Profiler::get().addGpuEvent(event.name.c_str(), durationUs, event.level);
                }
                
                // Release queries back to the heap
                pDevice->getTimestampQueryHeap()->release(event.startIndex);
                pDevice->getTimestampQueryHeap()->release(event.endIndex);
            }
        }

        frame.pResolveBuffer->unmap();

        // Process async events
        for (auto it = m_asyncEvents.begin(); it != m_asyncEvents.end(); )
        {
            if (it->isFinished)
            {
                // We need to map again or use the same pData
                // Let's re-map for simplicity or just keep it mapped.
                auto pDataAsync = static_cast<uint64_t const*>(frame.pResolveBuffer->map());
                uint64_t start = pDataAsync[it->startIndex];
                uint64_t end = pDataAsync[it->endIndex];
                frame.pResolveBuffer->unmap();

                if (end >= start && start > 0)
                {
                    double durationUs = static_cast<double>(end - start) * frequency * 1000.0;
                    core::Profiler::get().addGpuEvent(it->name.c_str(), durationUs, it->level);

                    pDevice->getTimestampQueryHeap()->release(it->startIndex);
                    pDevice->getTimestampQueryHeap()->release(it->endIndex);

                    it = m_asyncEvents.erase(it);
                    continue;
                }
            }
            ++it;
        }
        
        // After reading, we can mark it as consumed if needed, 
        // but resolve() will clear it next time.
    }

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
