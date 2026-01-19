#pragma once

#include "timers.hpp" // 请确保你的 PerformanceTimer 在这里

#include <cassert>
#include <cstdint>
#include <limits>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <mutex>
#include <list>
#include <array>
#include <utility> // for std::exchange
#include <source_location>

namespace april::core
{
    class ProfilerManager;

    class ProfilerTimeline
    {
    public:
        static constexpr uint32_t kMaxFrameDelay = 4;
        static constexpr uint32_t kMaxLastFrames = 128;
        static constexpr uint32_t kLevelSingleshot = std::numeric_limits<uint32_t>::max();

        // -----------------------------------------------------------------------------
        // ID Structures with Sentinel Values (No bool flag needed)
        // -----------------------------------------------------------------------------

        static constexpr uint32_t kInvalidFrameId = (1 << 28) - 1; // 28-bit max
        static constexpr uint32_t kInvalidAsyncId = std::numeric_limits<uint32_t>::max();

        struct FrameSectionID
        {
            uint32_t id : 28 = kInvalidFrameId; // Default to invalid
            uint32_t subFrame : 4 = 0;

            [[nodiscard]] constexpr bool isValid() const { return id != kInvalidFrameId; }
        };

        struct AsyncSectionID
        {
            uint32_t id = kInvalidAsyncId; // Default to invalid

            [[nodiscard]] constexpr bool isValid() const { return id != kInvalidAsyncId; }
        };

        struct CreateInfo
        {
            std::string name = "Main";
            uint32_t frameConfigDelay = 8;
            uint32_t defaultTimers = 128;
            uint32_t frameDelay = kMaxFrameDelay;
            uint32_t frameAveragingCount = kMaxLastFrames;
        };

        using gpuFrameTimeProvider_fn = std::function<bool(FrameSectionID, double& gpuTime)>;
        using gpuAsyncTimeProvider_fn = std::function<bool(AsyncSectionID, double& gpuTime)>;

        struct GpuTimeProvider
        {
            std::string             apiName;
            gpuFrameTimeProvider_fn frameFunction;
            gpuAsyncTimeProvider_fn asyncFunction;

            static auto getTimerBaseIdx(FrameSectionID slot) -> uint32_t
            {
                return ((slot.id * kMaxFrameDelay) + slot.subFrame) * 2;
            }
            static auto getTimerBaseIdx(AsyncSectionID slot) -> uint32_t { return (slot.id * 2); }
        };

        // -----------------------------------------------------------------------------
        // Public API
        // -----------------------------------------------------------------------------

        auto frameAdvance() -> void;

        // Use string_view for zero-copy
        auto frameBeginSection(std::string_view name, GpuTimeProvider* gpuTimeProvider = nullptr, std::source_location loc = std::source_location::current()) -> FrameSectionID;
        auto frameEndSection(FrameSectionID sec) -> void;
        auto frameResetCpuBegin(FrameSectionID sec) -> void;
        auto frameAccumulationSplit() -> void;

        auto asyncBeginSection(std::string_view name, GpuTimeProvider* gpuTimeProvider = nullptr) -> AsyncSectionID;
        auto asyncEndSection(AsyncSectionID sec) -> void;
        auto asyncResetCpuBegin(AsyncSectionID sec) -> void;
        auto asyncRemoveTimer(std::string_view name) -> void;

        // -----------------------------------------------------------------------------
        // Data Structures for Stats
        // -----------------------------------------------------------------------------

        struct TimerStats
        {
            double   last        = 0;
            double   average     = 0;
            double   absMinValue = 0;
            double   absMaxValue = 0;
            uint32_t index       = 0;
            std::array<double, kMaxLastFrames> times = {};
        };

        struct TimerInfo
        {
            uint32_t numAveraged = 0;
            bool accumulated = false;
            bool async       = false;
            uint32_t level = 0;
            TimerStats cpu;
            TimerStats gpu;
        };

        class Snapshot
        {
        public:
            std::string name;
            size_t id = 0;
            std::vector<TimerInfo> timerInfos;
            std::vector<std::string> timerNames;
            std::vector<std::string> timerApiNames;

            auto appendToString(std::string& stats, bool full) const -> void;
        };

        // Getters
        [[nodiscard]] auto getName() const -> std::string const& { return m_info.name; }
        [[nodiscard]] auto getProfiler() const -> ProfilerManager* { return m_profiler; }

        auto getAsyncSnapshot(Snapshot& snapShot) const -> void;
        auto getAsyncTimerInfo(std::string_view name, TimerInfo& timerInfo, std::string& apiName) const -> bool;
        auto getFrameSnapshot(Snapshot& snapShot) const -> void;
        auto getFrameTimerInfo(std::string_view name, TimerInfo& info, std::string& apiName) const -> bool;

        // Configuration
        auto clear() -> void;
        auto resetFrameSections(uint32_t delay = 0) -> void;
        auto setFrameAveragingCount(uint32_t num) -> void;

        // -----------------------------------------------------------------------------
        // Elegant RAII Sections (Sentinel Value Pattern)
        // -----------------------------------------------------------------------------

        class FrameSection
        {
        public:
            FrameSection(ProfilerTimeline& timeline, FrameSectionID id)
                : m_ProfilerTimeline(timeline), m_id(id) {}

            FrameSection(FrameSection const&) = delete;
            auto operator=(FrameSection const&) -> FrameSection& = delete;

            // Move Constructor: Exchange ID with invalid one
            FrameSection(FrameSection&& other) noexcept
                : m_ProfilerTimeline(other.m_ProfilerTimeline)
                , m_id(std::exchange(other.m_id, {kInvalidFrameId}))
            {}

            ~FrameSection() {
                if (m_id.isValid()) m_ProfilerTimeline.frameEndSection(m_id);
            }

        private:
            ProfilerTimeline& m_ProfilerTimeline;
            FrameSectionID    m_id;
        };

        [[nodiscard]] auto frameSection(
            std::string_view name,
            std::source_location loc = std::source_location::current()
        ) -> FrameSection
        {
            return FrameSection(*this, frameBeginSection(name, nullptr, loc));
        }

        class AsyncSection
        {
        public:
            AsyncSection(ProfilerTimeline& timeline, AsyncSectionID id)
                : m_ProfilerTimeline(timeline), m_id(id) {}

            AsyncSection(AsyncSection const&) = delete;
            auto operator=(AsyncSection const&) -> AsyncSection& = delete;

            // Move Constructor: Exchange ID with invalid one
            AsyncSection(AsyncSection&& other) noexcept
                : m_ProfilerTimeline(other.m_ProfilerTimeline)
                , m_id(std::exchange(other.m_id, {kInvalidAsyncId}))
            {}

            ~AsyncSection() {
                if (m_id.isValid()) m_ProfilerTimeline.asyncEndSection(m_id);
            }

        private:
            ProfilerTimeline& m_ProfilerTimeline;
            AsyncSectionID    m_id;
        };

        [[nodiscard]] auto asyncSection(std::string_view name) -> AsyncSection
        {
            return AsyncSection(*this, asyncBeginSection(name, nullptr));
        }

        ProfilerTimeline() = default;

    protected:
        friend class ProfilerManager;

        ProfilerTimeline(ProfilerManager* profiler, ProfilerTimeline::CreateInfo const& createInfo);

        auto frameBegin() -> void;
        auto frameEnd() -> void;

        auto frameGetSectionID() -> FrameSectionID;
        auto frameGetTimerInfo(uint32_t i, TimerInfo& info) -> bool;
        auto frameInternalSnapshot() -> void;
        auto asyncGetTimerInfo(uint32_t i, TimerInfo& info) const -> bool;

        // -----------------------------------------------------------------------------
        // Internals
        // -----------------------------------------------------------------------------

        struct TimeValues
        {
            double valueLast   = 0;
            double valueTotal  = 0;
            double absMinValue = std::numeric_limits<double>::max();
            double absMaxValue = 0;

            uint32_t cycleIndex = 0;
            uint32_t cycleCount = kMaxLastFrames;
            uint32_t validCount = 0;

            std::array<double, kMaxLastFrames> times = {};

            TimeValues(uint32_t averagedFrameCount_ = kMaxLastFrames) { init(averagedFrameCount_); }

            auto init(uint32_t averagedFrameCount_) -> void;
            auto reset() -> void;
            auto add(double time) -> void;
            [[nodiscard]] auto getAveraged() const -> double;
        };

        struct SectionData
        {
            // Holds string_view (Zero Copy). Name MUST be persistent (e.g., literal).
            std::string_view name = {};
            GpuTimeProvider* gpuTimeProvider = nullptr;

            uint32_t level    = 0;
            uint32_t subFrame = 0;

            std::array<double, kMaxFrameDelay> cpuTimes = {};
            std::array<double, kMaxFrameDelay> gpuTimes = {};

            uint32_t numTimes = 0;
            TimeValues gpuTime;
            TimeValues cpuTime;

            bool splitter = false;
            bool accumulated = false;
        };

        struct FrameData
        {
            uint32_t averagingCount     = kMaxLastFrames;
            uint32_t averagingCountLast = kMaxLastFrames;
            uint32_t resetDelay = 0;
            uint32_t count = 0;
            uint32_t countLastReset = 0;

            bool     hasSplitter = false;
            uint32_t level       = 0;
            uint32_t sectionsCount     = 0;
            uint32_t sectionsCountLast = 0;

            double     cpuCurrentTime = 0;
            TimeValues cpuTime;
            TimeValues gpuTime;

            std::vector<SectionData> sections;
        };

        struct AsyncData
        {
            uint32_t                 sectionsCount = 0;
            std::vector<SectionData> sections;
        };

        auto grow(std::vector<SectionData>& sections, size_t newSize, uint32_t averagingCount) -> void;

        bool                 m_inFrame = false;
        ProfilerManager* m_profiler{};
        ProfilerTimeline::CreateInfo m_info;
        FrameData            m_frame;
        Snapshot             m_frameSnapshot;
        mutable std::mutex   m_frameSnapshotMutex;
        AsyncData            m_async;
        mutable std::mutex   m_asyncMutex;
    };

    class ProfilerManager
    {
    public:
        ProfilerManager() = default;
        ~ProfilerManager();

        auto createTimeline(ProfilerTimeline::CreateInfo const& createInfo) -> ProfilerTimeline*;
        auto destroyTimeline(ProfilerTimeline* timeline) -> void;

        [[nodiscard]] auto getMicroseconds() const -> double { return m_timer.getMicroseconds(); }

        auto setFrameAveragingCount(uint32_t num) -> void;
        auto resetFrameSections(uint32_t delayInFrames) -> void;
        auto appendPrint(std::string& statsFrames, std::string& statsAsyncs, bool full = false) const -> void;
        auto getSnapshots(
            std::vector<ProfilerTimeline::Snapshot>& frameSnapshots,
            std::vector<ProfilerTimeline::Snapshot>& asyncSnapshots
        ) const -> void;

    protected:
        std::list<std::unique_ptr<ProfilerTimeline>> m_timelines;
        mutable std::mutex                           m_mutex;
        PerformanceTimer                             m_timer;
    };

    class GlobalProfiler
    {
    public:
        static auto init(std::string const& timelineName = "Main") -> void;
        static auto shutdown() -> void;
        static auto getManager() -> ProfilerManager*;
        static auto getTimeline() -> ProfilerTimeline*;
        [[nodiscard]] static auto scope(
            std::string_view name = {},
            std::source_location loc = std::source_location::current()
        ) -> std::unique_ptr<ProfilerTimeline::FrameSection>;
    };

}  // namespace april::core

// -----------------------------------------------------------------------------
// Macros (Modernized)
// -----------------------------------------------------------------------------
#define AP_PROFILE_SCOPE(name) \
   auto _ap_profile_scope_##__LINE__ = april::core::GlobalProfiler::scope(name)

#define AP_PROFILE_FUNCTION() auto _ap_profile_scope_##__LINE__ = april::core::GlobalProfiler::scope()