#pragma once

#include <core/profile/profiler.hpp>
#include <graphics/rhi/fwd.hpp>
#include <graphics/rhi/query-heap.hpp>
#include <graphics/rhi/fence.hpp>
#include <graphics/rhi/buffer.hpp>

#include <vector>
#include <string>
#include <queue>
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
            const char* name;
            uint32_t startQueryIndex;
            uint32_t endQueryIndex;
            uint32_t frameId;
        };

        struct FrameData
        {
            uint32_t frameId;
            core::ref<Fence> fence;
            std::vector<GpuEvent> events;
            uint32_t queryCount;
            uint64_t bufferOffset;
        };

        static auto create(core::ref<Device> pDevice) -> core::ref<GpuProfiler>;

        GpuProfiler(core::ref<Device> pDevice);
        virtual ~GpuProfiler();

        /**
         * Begins a GPU profiling zone.
         * @param pContext Command context to record timestamps in.
         * @param name Name of the zone.
         */
        auto beginZone(CommandContext* pContext, const char* name) -> void;

        /**
         * Ends a GPU profiling zone.
         * @param pContext Command context to record timestamps in.
         * @param name Name of the zone.
         */
        auto endZone(CommandContext* pContext, const char* name) -> void;

        /**
         * Called at the end of each frame to resolve queries and prepare for readback.
         */
        auto endFrame(CommandContext* pContext) -> void;

        /**
         * Implementation of IGpuProfiler::collectEvents.
         */
        auto collectEvents() -> std::vector<core::ProfileEvent> override;

    private:
        auto allocateQuery() -> uint32_t;
        auto releaseQuery(uint32_t index) -> void;

        core::BreakableReference<Device> mp_device;
        core::ref<Buffer> mp_readbackBuffer;
        
        std::vector<FrameData> m_frameData;
        uint32_t m_currentFrameIndex = 0;
        uint32_t m_activeFrameCount = 0;
        
        std::vector<GpuEvent> m_currentFrameEvents;
        uint32_t m_queriesUsedInFrame = 0;

        std::mutex m_eventMutex;
        std::vector<core::ProfileEvent> m_readyEvents;

        static constexpr uint32_t kMaxFramesInFlight = 3;
        static constexpr uint32_t kMaxQueriesPerFrame = 1024;
        static constexpr uint32_t kReadbackBufferSize = kMaxFramesInFlight * kMaxQueriesPerFrame * sizeof(uint64_t);
    };
}

/**
 * APRIL_GPU_ZONE macro for easy GPU profiling.
 */
#define APRIL_GPU_ZONE_CONCAT_INNER(a, b) a##b
#define APRIL_GPU_ZONE_CONCAT(a, b) APRIL_GPU_ZONE_CONCAT_INNER(a, b)

struct ScopedGpuProfileZone
{
    ScopedGpuProfileZone(april::graphics::CommandContext* pCtx, const char* name);
    ~ScopedGpuProfileZone();
    april::graphics::CommandContext* pContext;
    const char* m_name;
};

#define APRIL_GPU_ZONE(pContext, name) \
    ScopedGpuProfileZone APRIL_GPU_ZONE_CONCAT(gpu_unique_zone_, __LINE__)(pContext, name)