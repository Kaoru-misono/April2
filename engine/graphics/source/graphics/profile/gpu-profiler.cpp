#include "gpu-profiler.hpp"
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/fence.hpp>
#include <core/profile/timer.hpp>
#include <iostream>

namespace april::graphics
{
    GpuProfiler::GpuProfiler(core::ref<Device> pDevice)
        : mp_device(pDevice)
    {
        // Create readback buffer for timestamps
        mp_readbackBuffer = pDevice->createBuffer(
            kReadbackBufferSize,
            BufferUsage::None,
            MemoryType::ReadBack
        );

        m_frameData.resize(kMaxFramesInFlight);
        for (uint32_t i = 0; i < kMaxFramesInFlight; ++i)
        {
            m_frameData[i].fence = pDevice->createFence();
            m_frameData[i].bufferOffset = i * kMaxQueriesPerFrame * sizeof(uint64_t);
        }
    }

    GpuProfiler::~GpuProfiler() = default;

    auto GpuProfiler::create(core::ref<Device> pDevice) -> core::ref<GpuProfiler>
    {
        return core::make_ref<GpuProfiler>(pDevice);
    }

    auto GpuProfiler::beginZone(CommandContext* pContext, const char* name) -> void
    {
        uint32_t startIdx = allocateQuery();
        pContext->writeTimestamp(mp_device->getTimestampQueryHeap().get(), startIdx);

        GpuEvent event;
        event.name = name;
        event.startQueryIndex = startIdx;
        event.endQueryIndex = 0;
        event.frameId = m_activeFrameCount;
        m_currentFrameEvents.push_back(event);
    }

    auto GpuProfiler::endZone(CommandContext* pContext, const char* name) -> void
    {
        uint32_t endIdx = allocateQuery();
        pContext->writeTimestamp(mp_device->getTimestampQueryHeap().get(), endIdx);

        // Find matching event
        for (auto it = m_currentFrameEvents.rbegin(); it != m_currentFrameEvents.rend(); ++it)
        {
            if (std::string(it->name) == name && it->endQueryIndex == 0) // Basic matching
            {
                it->endQueryIndex = endIdx;
                break;
            }
        }
    }

    auto GpuProfiler::endFrame(CommandContext* pContext) -> void
    {
        auto& frame = m_frameData[m_currentFrameIndex];
        
        // Resolve queries to readback buffer
        if (m_queriesUsedInFrame > 0)
        {
            pContext->resolveQuery(
                mp_device->getTimestampQueryHeap().get(),
                0, 
                m_queriesUsedInFrame,
                mp_readbackBuffer.get(),
                frame.bufferOffset
            );
        }

        frame.events = std::move(m_currentFrameEvents);
        frame.queryCount = m_queriesUsedInFrame;
        frame.frameId = m_activeFrameCount++;
        
        // Signal fence
        pContext->signal(frame.fence.get());

        // Advance frame index
        m_currentFrameIndex = (m_currentFrameIndex + 1) % kMaxFramesInFlight;
        
        // Reset counters for next frame
        m_queriesUsedInFrame = 0;
        m_currentFrameEvents.clear();

        // Process completed frames (N+2)
        for (uint32_t i = 0; i < kMaxFramesInFlight; ++i)
        {
            auto& f = m_frameData[i];
            if (f.queryCount > 0 && f.fence->getCurrentValue() >= f.fence->getSignaledValue())
            {
                // Read back results
                uint64_t* pData = (uint64_t*)mp_readbackBuffer->map(rhi::CpuAccessMode::Read);
                uint64_t* pFrameTimestamps = pData + (f.bufferOffset / sizeof(uint64_t));

                std::lock_guard<std::mutex> lock(m_eventMutex);
                double freq = mp_device->getGpuTimestampFrequency(); // ms/tick

                for (const auto& e : f.events)
                {
                    uint64_t startTicks = pFrameTimestamps[e.startQueryIndex];
                    uint64_t endTicks = pFrameTimestamps[e.endQueryIndex];

                    core::ProfileEvent beginEvent;
                    beginEvent.name = e.name;
                    beginEvent.timestamp = static_cast<uint64_t>(startTicks * freq * 1000000.0); 
                    beginEvent.type = core::ProfileEventType::Begin;
                    beginEvent.threadId = 0xFFFFFFFF; // Special marker for GPU

                    core::ProfileEvent endEvent;
                    endEvent.name = e.name;
                    endEvent.timestamp = static_cast<uint64_t>(endTicks * freq * 1000000.0);
                    endEvent.type = core::ProfileEventType::End;
                    endEvent.threadId = 0xFFFFFFFF;

                    m_readyEvents.push_back(beginEvent);
                    m_readyEvents.push_back(endEvent);
                }

                mp_readbackBuffer->unmap();
                f.queryCount = 0; // Mark as processed
                f.events.clear();
            }
        }
    }

    auto GpuProfiler::collectEvents() -> std::vector<core::ProfileEvent>
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        return std::move(m_readyEvents);
    }

    auto GpuProfiler::allocateQuery() -> uint32_t
    {
        return m_queriesUsedInFrame++;
    }

    auto GpuProfiler::releaseQuery(uint32_t index) -> void
    {
        (void)index;
    }
}

// Scoped RAII helper implementation - defined in global scope as it is declared outside namespace in header
ScopedGpuProfileZone::ScopedGpuProfileZone(april::graphics::CommandContext* pCtx, const char* name)
    : pContext(pCtx), m_name(name)
{
    if (auto* pGpuProfiler = pCtx->getDevice()->getGpuProfiler())
    {
        pGpuProfiler->beginZone(pCtx, m_name);
    }
}

ScopedGpuProfileZone::~ScopedGpuProfileZone()
{
    if (auto* pGpuProfiler = pContext->getDevice()->getGpuProfiler())
    {
        pGpuProfiler->endZone(pContext, m_name);
    }
}