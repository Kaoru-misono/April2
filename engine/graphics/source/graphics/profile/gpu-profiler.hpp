#pragma once

#include <graphics/rhi/fwd.hpp>
#include <graphics/rhi/buffer.hpp>
#include <core/profile/profiler.hpp>

#include <string>
#include <vector>
#include <memory>

namespace april::graphics
{
    class Device;
    class CommandContext;

    class GpuProfiler : public core::Object
    {
        APRIL_OBJECT(GpuProfiler)
    public:
        static auto create(core::ref<Device> pDevice) -> core::ref<GpuProfiler>;
        virtual ~GpuProfiler();

        auto beginEvent(CommandContext* pContext, char const* name) -> void;
        auto endEvent(CommandContext* pContext) -> void;

        auto beginAsyncEvent(CommandContext* pContext, char const* name) -> uint32_t;
        auto endAsyncEvent(CommandContext* pContext, uint32_t asyncId) -> void;

        auto resolve(CommandContext* pContext) -> void;
        
        /**
         * Resolves timestamps to CPU time and feeds them back to core::Profiler.
         * Should be called when GPU results are available (usually with a frame delay).
         */
        auto update() -> void;

    private:
        GpuProfiler(core::ref<Device> pDevice);

        struct Event
        {
            std::string name;
            uint32_t startIndex = 0;
            uint32_t endIndex = 0;
            uint32_t level = 0;
        };

        struct FrameData
        {
            core::ref<Buffer> pResolveBuffer;
            core::ref<Buffer> pResolveStagingBuffer;
            std::vector<Event> events;
            uint32_t queryCount = 0;
            bool isResolved = false;
        };

        core::BreakableReference<Device> mp_device;
        std::vector<FrameData> m_frames;
        uint32_t m_currentFrameIndex = 0;
        uint32_t m_activeEventCount = 0;
        
        std::vector<uint32_t> m_eventStack;

        struct AsyncEvent : public Event
        {
            bool isFinished = false;
        };
        std::vector<AsyncEvent> m_asyncEvents;
    };

    struct ScopedGpuProfileEvent
    {
        ScopedGpuProfileEvent(CommandContext* pContext, char const* name);
        ~ScopedGpuProfileEvent();

    private:
        GpuProfiler* m_profiler = nullptr;
        CommandContext* m_context = nullptr;
    };


} // namespace april::graphics

#define AP_PROFILE_GPU_SCOPE(pContext, name) april::graphics::ScopedGpuProfileEvent AP_UNIQUE_VAR_NAME(gpu_profile_scope_)(pContext, name)
#define AP_PROFILE_GPU_FUNCTION(pContext) AP_PROFILE_GPU_SCOPE(pContext, std::source_location::current().function_name())
