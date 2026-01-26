#include "gpu-profiler.hpp"
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/fence.hpp>
#include <core/profile/timer.hpp>
#include <core/error/assert.hpp>
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
        mp_mappedReadback = mp_readbackBuffer->mapAs<uint64_t>(rhi::CpuAccessMode::Read);

        m_frameData.resize(kMaxFramesInFlight);
        for (uint32_t i = 0; i < kMaxFramesInFlight; ++i)
        {
            m_frameData[i].fenceValue = 0;
            m_frameData[i].bufferOffset = i * kMaxQueriesPerFrame * sizeof(uint64_t);
            m_frameData[i].cpuReferenceNs = 0;
            m_frameData[i].calibrationQueryIndex = 0xFFFFFFFF;
            m_frameData[i].queryCount = 0;
            m_frameData[i].queryBase = 0;
        }
    }

    GpuProfiler::~GpuProfiler()
    {
        if (mp_readbackBuffer && mp_mappedReadback)
        {
            mp_mappedReadback = nullptr;
        }
    }

    auto GpuProfiler::create(core::ref<Device> pDevice) -> core::ref<GpuProfiler>
    {
        return core::make_ref<GpuProfiler>(pDevice);
    }

    auto GpuProfiler::beginFrameCalibration(CommandContext* p_context) -> void
    {
        m_calibrationQueryIndex = allocateQuery();
        p_context->writeTimestamp(mp_device->getTimestampQueryHeap().get(), m_calibrationQueryIndex);
    }

    auto GpuProfiler::endFrameCalibration() -> void
    {
        // This is called just before submit in Device::endFrame to capture CPU time
        // Wait, the spec says "before and after submit".
        // I will handle the capture in Device::endFrame and pass it to a method.
    }

    auto GpuProfiler::beginZone(CommandContext* p_context, char const* name) -> void
    {
        uint32_t startIdx = allocateQuery();
        p_context->writeTimestamp(mp_device->getTimestampQueryHeap().get(), startIdx);

        GpuEvent event;
        event.name = name;
        event.startQueryIndex = startIdx;
        event.endQueryIndex = 0;
        event.frameId = m_activeFrameCount;
        m_currentFrameEvents.emplace_back(event);
    }

    auto GpuProfiler::endZone(CommandContext* p_context, char const* name) -> void
    {
        uint32_t endIdx = allocateQuery();
        p_context->writeTimestamp(mp_device->getTimestampQueryHeap().get(), endIdx);

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

    auto GpuProfiler::endFrame(CommandContext* p_context) -> void
    {
        auto& globalFence = mp_device->getGlobalFence();
        // 1. Process completed frames (from previous calls)
        for (uint32_t i = 0; i < kMaxFramesInFlight; ++i)
        {
            auto& f = m_frameData[i];
            if (f.queryCount > 0 && f.cpuReferenceNs != 0 && globalFence->getCurrentValue() >= f.fenceValue)
            {
                // Read back results
                uint64_t* pFrameTimestamps = mp_mappedReadback + (f.bufferOffset / sizeof(uint64_t));

                std::lock_guard<std::mutex> lock(m_eventMutex);
                double freq = mp_device->getGpuTimestampFrequency(); // ms/tick

                // Synchronization Logic
                const uint32_t frameBase = f.queryBase;
                const uint32_t frameEnd = frameBase + f.queryCount;
                if (f.calibrationQueryIndex != 0xFFFFFFFF &&
                    f.calibrationQueryIndex >= frameBase &&
                    f.calibrationQueryIndex < frameEnd)
                {
                    uint32_t localIndex = f.calibrationQueryIndex - frameBase;
                    uint64_t gpuBaseTicks = pFrameTimestamps[localIndex];
                    double gpuBaseNs = static_cast<double>(gpuBaseTicks) * freq * 1000000.0;
                    double currentOffset = static_cast<double>(f.cpuReferenceNs) - gpuBaseNs;

                    m_offsetHistory.emplace_back(currentOffset);
                    if (m_offsetHistory.size() > kMaxOffsetHistory)
                    {
                        m_offsetHistory.erase(m_offsetHistory.begin());
                    }

                    double sum = 0.0;
                    for (double off : m_offsetHistory) sum += off;
                    m_timeOffsetNs = sum / m_offsetHistory.size();
                }

                for (const auto& e : f.events)
                {
                    if (e.startQueryIndex < frameBase || e.startQueryIndex >= frameEnd ||
                        e.endQueryIndex < frameBase || e.endQueryIndex >= frameEnd)
                    {
                        continue;
                    }

                    uint32_t localStart = e.startQueryIndex - frameBase;
                    uint32_t localEnd = e.endQueryIndex - frameBase;
                    uint64_t startTicks = pFrameTimestamps[localStart];
                    uint64_t endTicks = pFrameTimestamps[localEnd];

                    auto translate = [&](uint64_t ticks) -> uint64_t {
                        double gpuNs = static_cast<double>(ticks) * freq * 1000000.0;
                        return static_cast<uint64_t>(gpuNs + m_timeOffsetNs);
                    };

                    core::ProfileEvent beginEvent;
                    beginEvent.name = e.name;
                    beginEvent.timestamp = translate(startTicks);
                    beginEvent.type = core::ProfileEventType::Begin;
                    beginEvent.threadId = 0xFFFFFFFF; // Special marker for GPU

                    core::ProfileEvent endEvent;
                    endEvent.name = e.name;
                    endEvent.timestamp = translate(endTicks);
                    endEvent.type = core::ProfileEventType::End;
                    endEvent.threadId = 0xFFFFFFFF;

                    m_readyEvents.emplace_back(beginEvent);
                    m_readyEvents.emplace_back(endEvent);
                }

                f.queryCount = 0; // Mark as processed
                f.cpuReferenceNs = 0;
                f.events.clear();
            }
        }

        // 2. Prepare current frame for submission
        auto& frame = m_frameData[m_currentFrameIndex];

        const uint32_t frameQueryBase = m_currentFrameIndex * kMaxQueriesPerFrame;
        frame.queryBase = frameQueryBase;

        // Resolve queries to readback buffer
        if (m_queriesUsedInFrame > 0)
        {
            p_context->resolveQuery(
                mp_device->getTimestampQueryHeap().get(),
                frameQueryBase,
                m_queriesUsedInFrame,
                mp_readbackBuffer.get(),
                frame.bufferOffset
            );
        }

        frame.events = std::move(m_currentFrameEvents);
        frame.queryCount = m_queriesUsedInFrame;
        frame.frameId = m_activeFrameCount++;
        frame.calibrationQueryIndex = m_calibrationQueryIndex;
        frame.cpuReferenceNs = 0; // Will be set by Device::endFrame after submit

        // Advance frame index
        m_currentFrameIndex = (m_currentFrameIndex + 1) % kMaxFramesInFlight;

        // Reset counters for next frame
        m_queriesUsedInFrame = 0;
        m_currentFrameEvents.clear();
        m_calibrationQueryIndex = 0xFFFFFFFF;
    }

    auto GpuProfiler::postSubmit(CommandContext* p_context, uint64_t cpuReferenceNs, uint64_t fenceValue) -> void
    {
        uint32_t lastIdx = (m_currentFrameIndex + kMaxFramesInFlight - 1) % kMaxFramesInFlight;
        m_frameData[lastIdx].cpuReferenceNs = cpuReferenceNs;
        m_frameData[lastIdx].fenceValue = fenceValue;

        beginFrameCalibration(p_context);
    }

    auto GpuProfiler::collectEvents() -> std::vector<core::ProfileEvent>
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        return std::move(m_readyEvents);
    }

    auto GpuProfiler::allocateQuery() -> uint32_t
    {
        AP_ASSERT(m_queriesUsedInFrame < kMaxQueriesPerFrame, "GpuProfiler exceeded per-frame query limit ({})", kMaxQueriesPerFrame);
        uint32_t frameBase = m_currentFrameIndex * kMaxQueriesPerFrame;
        return frameBase + m_queriesUsedInFrame++;
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
