#pragma once

#include "rhi/command-context.hpp"
#include <core/profile/profiler.hpp>
#include <graphics/rhi/fwd.hpp>
#include <graphics/rhi/query-heap.hpp>
#include <graphics/rhi/fence.hpp>
#include <graphics/rhi/buffer.hpp>

#include <array>
#include <vector>
#include <mutex>

namespace april::graphics
{
    /**
     * GPU Profiler implementation.
     * Manages timestamp queries and asynchronous readback.
     */
    class GpuProfiler : public core::IGpuProfiler, public core::Object
    {
        APRIL_OBJECT(GpuProfiler)
    public:
        struct GpuEvent
        {
            // NOTE: name must have static or long-lived storage; do not pass temporary/dynamic strings.
            char const* name;
            uint32_t startQueryIndex;
            uint32_t endQueryIndex;
            uint32_t frameId;
        };

        struct FrameData
        {
            uint32_t frameId;
            uint64_t fenceValue;
            std::vector<GpuEvent> events;
            uint32_t queryCount;
            uint32_t queryBase;
            uint64_t bufferOffset;
            uint64_t cpuReferenceNs;
            uint32_t calibrationQueryIndex;
        };

        static auto create(core::ref<Device> pDevice) -> core::ref<GpuProfiler>;

        GpuProfiler(core::ref<Device> pDevice);
        virtual ~GpuProfiler();

        /**
         * Begins frame calibration. Records a GPU timestamp.
         */
        auto beginFrameCalibration(CommandContext* p_context) -> void;

        /**
         * Ends frame calibration. Captures CPU reference time.
         */
        auto endFrameCalibration() -> void;

        /**
         * Begins a GPU profiling zone.
         * @param p_context Command context to record timestamps in.
         * @param name Name of the zone.
         */
        auto beginZone(CommandContext* p_context, char const* name) -> void;

        /**
         * Ends a GPU profiling zone.
         * @param p_context Command context to record timestamps in.
         * @param name Name of the zone.
         */
        auto endZone(CommandContext* p_context, char const* name) -> void;

        /**
         * Called at the end of each frame to resolve queries and prepare for readback.
         */
        auto endFrame(CommandContext* p_context) -> void;

        /**
         * Called after the submit to update the frame data.
         */
        auto postSubmit(CommandContext* p_context, uint64_t cpuReferenceNs, uint64_t fenceValue) -> void;

        /**
         * Implementation of IGpuProfiler::collectEvents.
         */
        auto collectEvents() -> std::vector<core::ProfileEvent> override;

    private:
        auto allocateQuery() -> uint32_t;
        auto releaseQuery(uint32_t index) -> void;

        static constexpr uint32_t kMaxFramesInFlight = Device::kInFlightFrameCount;
        static constexpr uint32_t kMaxQueriesPerFrame = 1024;
        static constexpr uint32_t kReadbackBufferSize = kMaxFramesInFlight * kMaxQueriesPerFrame * sizeof(uint64_t);

        core::BreakableReference<Device> mp_device;
        std::array<core::ref<QueryHeap>, kMaxFramesInFlight> m_queryHeaps;
        core::ref<Buffer> mp_resolveBuffer;
        core::ref<Buffer> mp_readbackBuffer;
        uint64_t* mp_mappedReadback = nullptr;

        std::vector<FrameData> m_frameData;
        uint32_t m_currentFrameIndex = 0;
        uint32_t m_activeFrameCount = 0;

        std::vector<GpuEvent> m_currentFrameEvents;
        uint32_t m_queriesUsedInFrame = 0;
        uint32_t m_calibrationQueryIndex = 0xFFFFFFFF;

        std::mutex m_eventMutex;
        std::vector<core::ProfileEvent> m_readyEvents;

        // Synchronization state
        double m_timeOffsetNs = 0.0;
        std::vector<double> m_offsetHistory;
        static constexpr size_t kMaxOffsetHistory = 32;
    };
}

/**
 * APRIL_GPU_ZONE macro for easy GPU profiling.
 */
#define APRIL_GPU_ZONE_CONCAT_INNER(a, b) a##b
#define APRIL_GPU_ZONE_CONCAT(a, b) APRIL_GPU_ZONE_CONCAT_INNER(a, b)

struct ScopedGpuProfileZone
{
    ScopedGpuProfileZone(april::graphics::CommandContext* pCtx, char const* name);
    ~ScopedGpuProfileZone();
    april::graphics::CommandContext* pContext;
    char const* m_name;
};

#define APRIL_GPU_ZONE(pContext, name) \
    ScopedGpuProfileZone APRIL_GPU_ZONE_CONCAT(gpu_unique_zone_, __LINE__)(pContext, name)
