#include "gpu-profiler.hpp"
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/fence.hpp>
#include <core/profile/timer.hpp>
#include <core/error/assert.hpp>
#include <core/log/logger.hpp>

#include <cstring>

namespace april::graphics
{
    GpuProfiler::GpuProfiler(core::ref<Device> pDevice)
        : mp_device(pDevice)
    {
        for (auto& heap : m_queryHeaps)
        {
            heap = QueryHeap::create(pDevice, QueryHeap::Type::Timestamp, kMaxQueriesPerFrame);
            heap->breakStrongReferenceToDevice();
        }

        mp_resolveBuffer = pDevice->createBuffer(
            kReadbackBufferSize,
            BufferUsage::UnorderedAccess | BufferUsage::CopySource | BufferUsage::CopyDestination,
            MemoryType::DeviceLocal
        );

        mp_readbackBuffer = pDevice->createBuffer(
            kReadbackBufferSize,
            BufferUsage::CopyDestination,
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
            mp_readbackBuffer->unmap();
            mp_mappedReadback = nullptr;
        }
    }

    auto GpuProfiler::create(core::ref<Device> pDevice) -> core::ref<GpuProfiler>
    {
        return core::make_ref<GpuProfiler>(pDevice);
    }

    auto GpuProfiler::beginFrameCalibration(CommandContext* p_context) -> void
    {
        auto& currentHeap = m_queryHeaps[m_currentFrameIndex];

        m_queriesUsedInFrame = 0;
        m_currentFrameEvents.clear();

        currentHeap->reset();

        auto& frame = m_frameData[m_currentFrameIndex];

        frame.calibrationQueryIndex = allocateQuery();

        p_context->writeTimestamp(currentHeap.get(), frame.calibrationQueryIndex);

        frame.queryCount = m_queriesUsedInFrame;
    }

    auto GpuProfiler::beginZone(CommandContext* p_context, char const* name) -> void
    {
        if (m_queriesUsedInFrame >= kMaxQueriesPerFrame) return;

        uint32_t startIdx = allocateQuery();
        p_context->writeTimestamp(m_queryHeaps[m_currentFrameIndex].get(), startIdx);

        GpuEvent event;
        event.name = name;
        event.startQueryIndex = startIdx;
        event.endQueryIndex = 0;
        event.frameId = m_activeFrameCount;
        m_currentFrameEvents.emplace_back(event);
    }

    auto GpuProfiler::endZone(CommandContext* p_context, char const* name) -> void
    {
        if (m_queriesUsedInFrame >= kMaxQueriesPerFrame) return;

        uint32_t endIdx = allocateQuery();
        p_context->writeTimestamp(m_queryHeaps[m_currentFrameIndex], endIdx);

        for (auto it = m_currentFrameEvents.rbegin(); it != m_currentFrameEvents.rend(); ++it)
        {
            if (it->endQueryIndex == 0 && strcmp(it->name, name) == 0)
            {
                it->endQueryIndex = endIdx;
                return;
            }
        }
        AP_WARN("GpuProfiler: Mismatched endZone for '{}'", name);
    }

    auto GpuProfiler::endFrame(CommandContext* p_context) -> void
    {
        auto& globalFence = mp_device->getGlobalFence();
        uint64_t completedValue = globalFence->getCurrentValue();

        // ---------------------------------------------------------
        // 1. (Readback)
        // ---------------------------------------------------------
        {
            std::lock_guard<std::mutex> lock(m_eventMutex);

            uint64_t freq = mp_device->getGpuTimestampFrequency();
            double nsPerTick = 1e9 / static_cast<double>(freq);

            for (uint32_t i = 0; i < kMaxFramesInFlight; ++i)
            {
                auto& f = m_frameData[i];

                if (f.queryCount > 0 && f.fenceValue > 0 && completedValue >= f.fenceValue)
                {
                    uint64_t* pTimestamps = mp_mappedReadback + (f.bufferOffset / sizeof(uint64_t));

                    uint64_t gpuCalibTick = pTimestamps[f.calibrationQueryIndex];
                    // TODO: make it really poison.
                    constexpr uint64_t kPoisonTick = 0xCDCDCDCDCDCDCDCDull;

                    if (gpuCalibTick == 0 || gpuCalibTick == kPoisonTick)
                    {
                        f.queryCount = 0;
                        f.fenceValue = 0;
                        f.events.clear();
                        continue;
                    }

                    if (gpuCalibTick > 0)
                    {
                        double gpuCalibNs = static_cast<double>(gpuCalibTick) * nsPerTick;
                        double offset = static_cast<double>(f.cpuReferenceNs) - gpuCalibNs;

                        if (m_timeOffsetNs == 0.0) m_timeOffsetNs = offset;
                        else m_timeOffsetNs = m_timeOffsetNs * 0.9 + offset * 0.1;
                    }

                    for (const auto& e : f.events)
                    {
                        if (e.endQueryIndex == 0) continue;

                        uint64_t tStart = pTimestamps[e.startQueryIndex];
                        uint64_t tEnd   = pTimestamps[e.endQueryIndex];

                        if (tStart == 0 || tEnd == 0 || tEnd < tStart) continue;

                        auto toCpuNs = [&](uint64_t ticks) -> double {
                            double gNs = static_cast<double>(ticks) * nsPerTick;
                            return gNs + m_timeOffsetNs;
                        };

                        core::ProfileEvent outEvent;
                        outEvent.name = e.name;
                        const double startNs = toCpuNs(tStart);
                        const double endNs = toCpuNs(tEnd);
                        outEvent.timestamp = startNs / 1000.0;
                        outEvent.duration = (endNs - startNs) / 1000.0;
                        if (outEvent.duration < 0.001)
                        {
                            outEvent.duration = 0.001;
                        }
                        outEvent.type = core::ProfileEventType::Complete;
                        outEvent.threadId = 0xFFFFFFFF; // GPU Thread ID
                        m_readyEvents.push_back(outEvent);
                    }

                    f.queryCount = 0;
                    f.fenceValue = 0;
                    f.events.clear();
                }
            }
        }

        // ---------------------------------------------------------
        // 2. (Resolve & Copy)
        // ---------------------------------------------------------
        auto& frame = m_frameData[m_currentFrameIndex];
        if (m_queriesUsedInFrame > 0)
        {
            p_context->bufferBarrier(mp_resolveBuffer, Resource::State::UnorderedAccess);
            p_context->resolveQuery(
                m_queryHeaps[m_currentFrameIndex],
                0,                      // Start Index
                m_queriesUsedInFrame,   // Count
                mp_resolveBuffer,
                frame.bufferOffset      // Dest Offset
            );

            p_context->bufferBarrier(mp_resolveBuffer, Resource::State::CopySource);
            p_context->bufferBarrier(mp_readbackBuffer, Resource::State::CopyDest);

            p_context->copyBuffer(
                mp_readbackBuffer,
                mp_resolveBuffer
            );
            p_context->bufferBarrier(mp_readbackBuffer, Resource::State::GenericRead);
        }

        frame.events = std::move(m_currentFrameEvents);
        frame.queryCount = m_queriesUsedInFrame;

        m_currentFrameIndex = (m_currentFrameIndex + 1) % kMaxFramesInFlight;

        m_queriesUsedInFrame = 0;
    }

    auto GpuProfiler::postSubmit(CommandContext* p_context, uint64_t cpuReferenceNs, uint64_t fenceValue) -> void
    {
        uint32_t submittedFrameIdx = (m_currentFrameIndex + kMaxFramesInFlight - 1) % kMaxFramesInFlight;
        m_frameData[submittedFrameIdx].fenceValue = fenceValue;
        m_frameData[submittedFrameIdx].cpuReferenceNs = cpuReferenceNs;

        beginFrameCalibration(p_context);
    }

    auto GpuProfiler::allocateQuery() -> uint32_t
    {
        AP_ASSERT(m_queriesUsedInFrame < kMaxQueriesPerFrame, "GpuProfiler exceeded per-frame query limit");
        return m_queriesUsedInFrame++;
    }

    auto GpuProfiler::collectEvents() -> std::vector<core::ProfileEvent>
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        std::vector<core::ProfileEvent> events = std::move(m_readyEvents);

        return events;
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
