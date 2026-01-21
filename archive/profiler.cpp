#include "profiler.hpp"

#include <core/error/assert.hpp>
#include <cassert>
#include <cstdint>
#include <format>
#include <limits>
#include <algorithm>

namespace april::core
{
    namespace
    {
        static std::unique_ptr<ProfilerManager> g_profilerManager = nullptr;
        static ProfilerTimeline* g_mainTimeline = nullptr; // Weak reference
        static constexpr uint32_t kInvalidLevel = std::numeric_limits<uint32_t>::max();
    }

    // -----------------------------------------------------------------------------
    // GlobalProfiler
    // -----------------------------------------------------------------------------

    auto GlobalProfiler::init(std::string const& timelineName) -> void
    {
        if (!g_profilerManager)
        {
            g_profilerManager = std::make_unique<ProfilerManager>();
            auto info = ProfilerTimeline::CreateInfo{};
            info.name = timelineName;
            g_mainTimeline = g_profilerManager->createTimeline(info);
        }
    }

    auto GlobalProfiler::shutdown() -> void
    {
        if (g_profilerManager)
        {
            g_profilerManager.reset(); // Calls destructor, destroys timelines
            g_mainTimeline = nullptr;
        }
    }

    auto GlobalProfiler::getManager() -> ProfilerManager*
    {
        return g_profilerManager.get();
    }

    auto GlobalProfiler::getTimeline() -> ProfilerTimeline*
    {
        return g_mainTimeline;
    }

    auto GlobalProfiler::scope(
        std::string_view name,
        std::source_location loc
    ) -> std::unique_ptr<ProfilerTimeline::FrameSection>
    {
        if (auto* timeline = getTimeline())
        {
            if (name.empty())
            {
                name = loc.function_name();
            }

            return std::make_unique<ProfilerTimeline::FrameSection>(timeline->frameSection(name, loc));
        }
        return nullptr;
    }

    // -----------------------------------------------------------------------------
    // ProfilerTimeline
    // -----------------------------------------------------------------------------

    ProfilerTimeline::ProfilerTimeline(ProfilerManager* profiler, ProfilerTimeline::CreateInfo const& createInfo)
    {
        assert(profiler);
        m_info     = createInfo;
        m_profiler = profiler;

        m_frame.averagingCount     = createInfo.frameAveragingCount;
        m_frame.averagingCountLast = createInfo.frameAveragingCount;

        grow(m_frame.sections, createInfo.defaultTimers, createInfo.frameAveragingCount);
        grow(m_async.sections, createInfo.defaultTimers, 0);

        frameBegin();
    }

    auto ProfilerTimeline::Snapshot::appendToString(std::string& stats, bool full) const -> void
    {
        auto const maxLevels = uint32_t{8};
        auto const maxLevel  = uint32_t{maxLevels - 1};
        using namespace std::literals;
        auto const spaces = "        "sv;

        auto foundMaxLevel = uint32_t{0};
        for(auto const& info : timerInfos) foundMaxLevel = std::max(foundMaxLevel, info.level);
        foundMaxLevel = std::min(foundMaxLevel, maxLevel);

        for(size_t i = 0; i < timerInfos.size(); i++)
        {
            auto const& info = timerInfos[i];
            auto level       = std::min(info.level, maxLevel);

            auto const* timerName = !timerNames[i].empty() ? timerNames[i].c_str() : "N/A";

            auto const indent = spaces.substr(0, std::min(spaces.size(), (size_t)(maxLevels - level)));
            auto const indentOp = spaces.substr(0, std::min(spaces.size(), (size_t)(maxLevels - (foundMaxLevel - level))));

            if(full)
            {
                stats += std::format("Timeline \"{}\"; level {}; Timer \"{}\"; GPU; avg {}; min {}; max {}; last {}; CPU; avg {}; min {}; max {}; last {}; samples {};\n",
                    name,
                    static_cast<int32_t>(info.async ? -1 : info.level),
                    timerName,
                    static_cast<uint32_t>(info.gpu.average),
                    static_cast<uint32_t>(info.gpu.absMinValue),
                    static_cast<uint32_t>(info.gpu.absMaxValue),
                    static_cast<uint32_t>(info.gpu.last),
                    static_cast<uint32_t>(info.cpu.average),
                    static_cast<uint32_t>(info.cpu.absMinValue),
                    static_cast<uint32_t>(info.cpu.absMaxValue),
                    static_cast<uint32_t>(info.cpu.last),
                    info.numAveraged);
            }
            else
            {
                stats += std::format("{:12}; {:3};{}{:16}{}; GPU; avg {:6}; CPU; avg {:6}; microseconds;\n",
                    name,
                    static_cast<int32_t>(info.async ? -1 : info.level),
                    indent, timerName, indentOp,
                    static_cast<uint32_t>(info.gpu.average),
                    static_cast<uint32_t>(info.cpu.average));
            }
        }
    }

    // -----------------------------------------------------------------------------
    // TimeValues Implementation
    // -----------------------------------------------------------------------------

    auto ProfilerTimeline::TimeValues::init(uint32_t averagedFrameCount_) -> void
    {
        cycleCount = std::min(averagedFrameCount_, kMaxLastFrames);
        reset();
    }

    auto ProfilerTimeline::TimeValues::reset() -> void
    {
        valueTotal  = 0;
        valueLast   = 0;
        absMinValue = std::numeric_limits<double>::max();
        absMaxValue = 0;
        cycleIndex  = 0;
        validCount  = 0;
        times.fill(0.0);
    }

    auto ProfilerTimeline::TimeValues::add(double time) -> void
    {
        absMinValue = std::min(time, absMinValue);
        absMaxValue = std::max(time, absMaxValue);
        valueLast   = time;

        if(cycleCount > 0)
        {
            if (validCount >= cycleCount) valueTotal -= times[cycleIndex];

            valueTotal += time;
            validCount = std::min(validCount + 1, cycleCount);
        }
        else
        {
            valueTotal += time;
            validCount++;
        }

        times[cycleIndex] = time;
        cycleIndex = (cycleIndex + 1) % kMaxLastFrames;
    }

    auto ProfilerTimeline::TimeValues::getAveraged() const -> double
    {
        return validCount ? (valueTotal / static_cast<double>(validCount)) : 0.0;
    }

    // -----------------------------------------------------------------------------
    // ProfilerTimeline Logic
    // -----------------------------------------------------------------------------

    auto ProfilerTimeline::setFrameAveragingCount(uint32_t num) -> void
    {
        AP_ASSERT(num <= kMaxLastFrames);
        m_frame.averagingCount = num;
    }

    auto ProfilerTimeline::clear() -> void
    {
        {
            auto guard = std::lock_guard{m_asyncMutex};
            m_async.sections.clear();
            m_async.sectionsCount = 0;
        }
        {
            auto guard = std::lock_guard{m_frameSnapshotMutex};
            m_frameSnapshot = {};
        }
    }

    auto ProfilerTimeline::resetFrameSections(uint32_t delay) -> void
    {
        m_frame.resetDelay = delay ? delay : m_info.frameConfigDelay;
    }

    auto ProfilerTimeline::frameAccumulationSplit() -> void
    {
        AP_ASSERT(m_inFrame);
        auto sectionID = frameGetSectionID();
        m_frame.sections[sectionID.id].level    = m_frame.level;
        m_frame.sections[sectionID.id].splitter = true;
        m_frame.hasSplitter = true;
    }

    auto ProfilerTimeline::frameAdvance() -> void
    {
        if (m_inFrame)
        {
            frameEnd();
        }
        frameBegin();
    }

    auto ProfilerTimeline::frameBegin() -> void
    {
        m_frame.hasSplitter    = false;
        m_frame.level          = 1;
        m_frame.sectionsCount  = 0;
        m_frame.cpuCurrentTime = -m_profiler->getMicroseconds();
        m_inFrame              = true;
    }

    auto ProfilerTimeline::frameEnd() -> void
    {
        AP_ASSERT(m_frame.level == 1);
        AP_ASSERT(m_inFrame);

        m_frame.cpuCurrentTime += m_profiler->getMicroseconds();

        if(m_frame.sectionsCount && m_frame.sectionsCount != m_frame.sectionsCountLast)
        {
            m_frame.sectionsCountLast = m_frame.sectionsCount;
            m_frame.resetDelay        = m_info.frameConfigDelay;
        }

        if(m_frame.resetDelay)
        {
            m_frame.resetDelay--;
            for(size_t i = 0; i < m_frame.sections.size(); i++)
            {
                auto& section = m_frame.sections[i];
                section.numTimes     = 0;
                section.cpuTime.reset();
                section.gpuTime.reset();
            }
            m_frame.cpuTime.reset();
            m_frame.gpuTime.reset();
            m_frame.countLastReset = m_frame.count;
        }

        if(m_frame.averagingCount != m_frame.averagingCountLast)
        {
            for(size_t i = 0; i < m_frame.sections.size(); i++)
            {
                m_frame.sections[i].cpuTime.init(m_frame.averagingCount);
                m_frame.sections[i].gpuTime.init(m_frame.averagingCount);
            }
            m_frame.cpuTime.init(m_frame.averagingCount);
            m_frame.gpuTime.init(m_frame.averagingCount);
            m_frame.averagingCountLast = m_frame.averagingCount;
        }

        if((m_frame.count - m_frame.countLastReset) > m_info.frameDelay)
        {
            auto gpuTime      = double{0};
            auto gpuLastLevel = kInvalidLevel;

            for(uint32_t i = 0; i < m_frame.sectionsCount; i++)
            {
                auto& section = m_frame.sections[i];
                if(section.splitter) continue;

                auto queryFrame = (m_frame.count + 1) % m_info.frameDelay;
                auto sec = FrameSectionID{};
                sec.id       = i;
                sec.subFrame = queryFrame;

                auto available = !section.gpuTimeProvider || section.gpuTimeProvider->frameFunction(sec, section.gpuTimes[queryFrame]);

                if(gpuLastLevel != kInvalidLevel && section.level < gpuLastLevel)
                    gpuLastLevel = kInvalidLevel;

                if(available)
                {
                    section.cpuTime.add(section.cpuTimes[queryFrame]);
                    section.gpuTime.add(section.gpuTimes[queryFrame]);
                    section.numTimes++;

                    if(gpuLastLevel == kInvalidLevel || gpuLastLevel == section.level)
                    {
                        gpuTime += section.gpuTimes[queryFrame];
                        gpuLastLevel = section.level;
                    }
                }
            }
            m_frame.gpuTime.add(gpuTime);
            m_frame.cpuTime.add(m_frame.cpuCurrentTime);
        }

        frameInternalSnapshot();
        m_frame.count++;
        m_inFrame = false;
    }

    auto ProfilerTimeline::frameInternalSnapshot() -> void
    {
        auto lock = std::lock_guard{m_frameSnapshotMutex};

        m_frameSnapshot.timerInfos.clear();
        m_frameSnapshot.timerNames.clear();
        m_frameSnapshot.timerApiNames.clear();
        m_frameSnapshot.name = m_info.name;
        m_frameSnapshot.id   = (size_t)this;

        auto info = TimerInfo{};

        // Frame Totals
        info.cpu.last        = m_frame.cpuTime.valueLast;
        info.cpu.average     = m_frame.cpuTime.getAveraged();
        info.cpu.absMaxValue = m_frame.cpuTime.absMaxValue;
        info.cpu.absMinValue = m_frame.cpuTime.absMinValue;
        info.cpu.times       = m_frame.cpuTime.times;
        info.cpu.index       = m_frame.cpuTime.cycleIndex;

        info.gpu.last        = m_frame.gpuTime.valueLast;
        info.gpu.average     = m_frame.gpuTime.getAveraged();
        info.gpu.absMaxValue = m_frame.gpuTime.absMaxValue;
        info.gpu.absMinValue = m_frame.gpuTime.absMinValue;
        info.gpu.times       = m_frame.gpuTime.times;
        info.gpu.index       = m_frame.gpuTime.cycleIndex;

        info.numAveraged = m_frame.cpuTime.validCount;

        if(m_frame.cpuTime.validCount)
        {
            m_frameSnapshot.timerInfos.push_back(info);
            m_frameSnapshot.timerNames.emplace_back("Frame");
            m_frameSnapshot.timerApiNames.emplace_back("GPU");
        }

        // Reset accumulated flags
        for(uint32_t i = 0; i < m_frame.sectionsCountLast; i++)
            m_frame.sections[i].accumulated = false;

        for(uint32_t i = 0; i < m_frame.sectionsCountLast; i++)
        {
            auto& section = m_frame.sections[i];
            if(section.splitter) continue;

            if(frameGetTimerInfo(i, info))
            {
                m_frameSnapshot.timerInfos.push_back(info);
                // Convert view to string for snapshot storage
                m_frameSnapshot.timerNames.emplace_back(section.name);
                m_frameSnapshot.timerApiNames.emplace_back(section.gpuTimeProvider ? section.gpuTimeProvider->apiName : "");
            }
        }
    }

    auto ProfilerTimeline::frameGetSectionID() -> FrameSectionID
    {
        auto sec = FrameSectionID{};
        assert(m_inFrame);

        sec.id       = m_frame.sectionsCount++;
        sec.subFrame = m_frame.count % m_info.frameDelay;

        if(sec.id >= m_frame.sections.size())
        {
            grow(m_frame.sections, m_frame.sections.size() * 2, m_frame.averagingCountLast);
        }
        return sec;
    }

    auto ProfilerTimeline::frameBeginSection(
        std::string_view name,
        GpuTimeProvider* gpuTimeProvider,
        std::source_location loc
    ) -> FrameSectionID
    {
        if (name.empty()) name = loc.function_name();

        auto sectionID = frameGetSectionID();
        auto&   section   = m_frame.sections[sectionID.id];
        auto       level     = m_frame.level++;

        if(section.name != name || section.gpuTimeProvider != gpuTimeProvider || section.level != level)
        {
            section.name            = name;
            section.gpuTimeProvider = gpuTimeProvider;

            // section.fileName = loc.file_name();
            // section.lineNumber = loc.line();

            m_frame.resetDelay = m_info.frameConfigDelay;
        }

        section.subFrame        = sectionID.subFrame;
        section.level           = level;
        section.splitter        = false;
        section.gpuTimeProvider = gpuTimeProvider;

        section.cpuTimes[sectionID.subFrame] = -m_profiler->getMicroseconds();
        section.gpuTimes[sectionID.subFrame] = 0;

        return sectionID;
    }

    auto ProfilerTimeline::frameEndSection(FrameSectionID sectionID) -> void
    {
        auto& section = m_frame.sections[sectionID.id];
        section.cpuTimes[sectionID.subFrame] += m_profiler->getMicroseconds();
        m_frame.level--;
    }

    auto ProfilerTimeline::frameResetCpuBegin(FrameSectionID sectionID) -> void
    {
        auto& section = m_frame.sections[sectionID.id];
        section.cpuTimes[sectionID.subFrame] = -m_profiler->getMicroseconds();
    }

    auto ProfilerTimeline::asyncBeginSection(std::string_view name, GpuTimeProvider* gpuTimeProvider) -> AsyncSectionID
    {
        auto lock = std::lock_guard{m_asyncMutex};
        auto sectionID = AsyncSectionID{};
        auto found     = false;

        for(uint32_t i = 0; i < m_async.sectionsCount; i++)
        {
            auto& section = m_async.sections[i];
            if(section.name.empty() || section.name == name)
            {
                sectionID.id = i;
                found        = true;
                break;
            }
        }

        if(!found)
        {
            sectionID.id = m_async.sectionsCount++;
            if(sectionID.id >= m_async.sections.size())
                grow(m_async.sections, m_async.sections.size() * 2, 0);
        }

        auto& section    = m_async.sections[sectionID.id];
        section.name            = name;
        section.gpuTimeProvider = gpuTimeProvider;
        section.subFrame        = 0;
        section.level           = kLevelSingleshot;
        section.splitter        = false;
        section.gpuTimeProvider = gpuTimeProvider;
        section.cpuTimes[0] = -m_profiler->getMicroseconds();
        section.gpuTimes[0] = 0;

        return sectionID;
    }

    auto ProfilerTimeline::asyncEndSection(ProfilerTimeline::AsyncSectionID sectionID) -> void
    {
        auto endTime = m_profiler->getMicroseconds();
        auto lock = std::lock_guard{m_asyncMutex};
        if(sectionID.id < m_async.sectionsCount)
        {
            auto& section = m_async.sections[sectionID.id];
            section.cpuTimes[0] += endTime;
            section.numTimes = 1;
        }
    }

    auto ProfilerTimeline::asyncResetCpuBegin(AsyncSectionID sectionID) -> void
    {
        auto lock = std::lock_guard{m_asyncMutex};
        if(sectionID.id < m_async.sectionsCount)
        {
            auto& section = m_async.sections[sectionID.id];
            section.cpuTimes[0] = -m_profiler->getMicroseconds();
        }
    }

    auto ProfilerTimeline::asyncRemoveTimer(std::string_view name) -> void
    {
        auto lock = std::lock_guard{m_asyncMutex};
        for(uint32_t i = 0; i < m_async.sectionsCount; i++)
        {
            auto& section = m_async.sections[i];
            if(section.name == name)
            {
                section.name = {};
                section.cpuTime.validCount = 0;
                if(i == m_async.sectionsCount - 1)
                    m_async.sectionsCount--;
                return;
            }
        }
    }

    auto ProfilerTimeline::grow(std::vector<SectionData>& sections, size_t newSize, uint32_t averagingCount) -> void
    {
        if(sections.size() >= newSize) return;
        size_t oldSize = sections.size();
        sections.resize(newSize);
        for(size_t i = oldSize; i < newSize; i++)
        {
            sections[i].cpuTime.init(averagingCount);
            sections[i].gpuTime.init(averagingCount);
        }
    }

    auto ProfilerTimeline::frameGetTimerInfo(uint32_t i, TimerInfo& info) -> bool
    {
        auto& section = m_frame.sections[i];
        if(!section.numTimes || section.accumulated) return false;

        info.accumulated     = false;
        info.async           = false;
        info.level           = section.level;
        info.cpu.last        = section.cpuTime.valueLast;
        info.gpu.last        = section.gpuTime.valueLast;
        info.gpu.average     = section.gpuTime.getAveraged();
        info.cpu.average     = section.cpuTime.getAveraged();
        info.cpu.absMinValue = section.cpuTime.absMinValue;
        info.cpu.absMaxValue = section.cpuTime.absMaxValue;
        info.gpu.absMinValue = section.gpuTime.absMinValue;
        info.gpu.absMaxValue = section.gpuTime.absMaxValue;
        info.cpu.times       = section.cpuTime.times;
        info.gpu.times       = section.gpuTime.times;
        info.cpu.index       = section.cpuTime.cycleIndex;
        info.gpu.index       = section.gpuTime.cycleIndex;

        auto found = false;
        if(section.level != kLevelSingleshot && m_frame.hasSplitter)
        {
            for(uint32_t n = i + 1; n < m_frame.sectionsCountLast; n++)
            {
                auto& otherSection = m_frame.sections[n];
                // string_view comparison
                if(otherSection.name == section.name
                    && otherSection.level == section.level
                    && otherSection.gpuTimeProvider == section.gpuTimeProvider
                    && !otherSection.accumulated)
                {
                    found = true;
                    info.cpu.last += otherSection.cpuTime.valueLast;
                    info.gpu.last += otherSection.gpuTime.valueLast;
                    info.gpu.average += otherSection.gpuTime.getAveraged();
                    info.cpu.average += otherSection.cpuTime.getAveraged();
                    info.cpu.absMinValue += otherSection.cpuTime.absMinValue;
                    info.cpu.absMaxValue += otherSection.cpuTime.absMaxValue;
                    info.gpu.absMinValue += otherSection.gpuTime.absMinValue;
                    info.gpu.absMaxValue += otherSection.gpuTime.absMaxValue;
                    otherSection.accumulated = true;
                }
                if(otherSection.splitter && otherSection.level <= section.level) break;
            }
        }
        info.accumulated = found;
        info.numAveraged = section.cpuTime.validCount;
        return true;
    }

    auto ProfilerTimeline::asyncGetTimerInfo(uint32_t i, TimerInfo& info) const -> bool
    {
        auto const& section = m_async.sections[i];
        auto sectionID = AsyncSectionID{};
        sectionID.id = i;

        auto cpuTime   = section.cpuTimes[0];
        auto gpuTime   = double{0};
        auto available = (!section.gpuTimeProvider || section.gpuTimeProvider->asyncFunction(sectionID, gpuTime));

        if(available)
        {
            info.accumulated = false;
            info.async       = true;
            info.numAveraged = 1;
            info.level       = 0;

            // BUG FIX: Correctly assigning Min and Max
            info.cpu.absMaxValue = cpuTime;
            info.cpu.absMinValue = cpuTime;
            info.cpu.average     = cpuTime;
            info.cpu.last        = cpuTime;

            info.gpu.absMaxValue = gpuTime;
            info.gpu.absMinValue = gpuTime;
            info.gpu.average     = gpuTime;
            info.gpu.last        = gpuTime;

            return true;
        }
        return false;
    }

    auto ProfilerTimeline::getAsyncSnapshot(Snapshot& snapShot) const -> void
    {
        snapShot.name = m_info.name;
        snapShot.id   = (size_t)this;
        snapShot.timerInfos.clear();
        snapShot.timerNames.clear();
        snapShot.timerApiNames.clear();

        auto lock = std::lock_guard{m_asyncMutex};
        snapShot.timerInfos.push_back({});
        snapShot.timerNames.emplace_back("Async");
        snapShot.timerApiNames.emplace_back("GPU");

        for(uint32_t i = 0; i < m_async.sectionsCount; i++)
        {
            auto const& section = m_async.sections[i];
            if(section.name.empty()) continue;

            auto timerInfo = TimerInfo{};
            if(asyncGetTimerInfo(i, timerInfo))
            {
                timerInfo.level++;
                snapShot.timerInfos.push_back(timerInfo);
                snapShot.timerNames.emplace_back(section.name);
                snapShot.timerApiNames.emplace_back(section.gpuTimeProvider ? section.gpuTimeProvider->apiName : "");
            }
        }
        if(snapShot.timerInfos.size() == 1)
        {
            snapShot.timerInfos.clear();
            snapShot.timerNames.clear();
            snapShot.timerApiNames.clear();
        }
    }

    auto ProfilerTimeline::getAsyncTimerInfo(std::string_view name, TimerInfo& timerInfo, std::string& apiName) const -> bool
    {
        auto lock = std::lock_guard{m_asyncMutex};
        for(uint32_t i = 0; i < m_async.sectionsCount; i++)
        {
            auto const& section = m_async.sections[i];
            if(section.name != name) continue;
            if(section.gpuTimeProvider) apiName = section.gpuTimeProvider->apiName;
            return asyncGetTimerInfo(i, timerInfo);
        }
        return false;
    }

    auto ProfilerTimeline::getFrameSnapshot(Snapshot& snapShot) const -> void
    {
        auto lock = std::lock_guard{m_frameSnapshotMutex};
        snapShot = m_frameSnapshot;
    }

    auto ProfilerTimeline::getFrameTimerInfo(std::string_view name, TimerInfo& info, std::string& apiName) const -> bool
    {
        auto lock = std::lock_guard{m_frameSnapshotMutex};
        for(size_t i = 0; i < m_frameSnapshot.timerNames.size(); i++)
        {
            if(m_frameSnapshot.timerNames[i] == name)
            {
                apiName = m_frameSnapshot.timerApiNames[i];
                info    = m_frameSnapshot.timerInfos[i];
                return true;
            }
        }
        return false;
    }

    // -----------------------------------------------------------------------------
    // ProfilerManager
    // -----------------------------------------------------------------------------

    ProfilerManager::~ProfilerManager() = default;

    auto ProfilerManager::createTimeline(ProfilerTimeline::CreateInfo const& createInfo) -> ProfilerTimeline*
    {
        auto lock = std::lock_guard{m_mutex};
        // Use make_unique for exception safety
        return m_timelines.emplace_back(std::unique_ptr<ProfilerTimeline>(new ProfilerTimeline(this, createInfo))).get();
    }

    auto ProfilerManager::destroyTimeline(ProfilerTimeline* timeline) -> void
    {
        auto lock = std::lock_guard{m_mutex};
        for(auto it = m_timelines.begin(); it != m_timelines.end(); it++)
        {
            if(it->get() == timeline)
            {
                m_timelines.erase(it);
                return;
            }
        }
        assert(0 && "invalid timeline");
    }

    auto ProfilerManager::setFrameAveragingCount(uint32_t num) -> void
    {
        auto lock = std::lock_guard{m_mutex};
        for(auto& it : m_timelines) it->setFrameAveragingCount(num);
    }

    auto ProfilerManager::resetFrameSections(uint32_t delayInFrames) -> void
    {
        auto lock = std::lock_guard{m_mutex};
        for(auto& it : m_timelines) it->resetFrameSections(delayInFrames);
    }

    auto ProfilerManager::appendPrint(std::string& statsFrames, std::string& statsAsyncs, bool full) const -> void
    {
        auto frameSnapshots = std::vector<ProfilerTimeline::Snapshot>{};
        auto asyncSnapshots = std::vector<ProfilerTimeline::Snapshot>{};
        getSnapshots(frameSnapshots, asyncSnapshots);

        for(auto& it : frameSnapshots) it.appendToString(statsFrames, full);
        for(auto& it : asyncSnapshots) it.appendToString(statsAsyncs, full);
    }

    auto ProfilerManager::getSnapshots(
        std::vector<ProfilerTimeline::Snapshot>& frameSnapshots,
        std::vector<ProfilerTimeline::Snapshot>& asyncSnapshots
    ) const -> void
    {
        auto lock = std::lock_guard{m_mutex};
        frameSnapshots.resize(m_timelines.size());
        asyncSnapshots.resize(m_timelines.size());
        auto i = size_t{0};
        for(auto& it : m_timelines)
        {
            it->getFrameSnapshot(frameSnapshots[i]);
            it->getAsyncSnapshot(asyncSnapshots[i]);
            i++;
        }
    }
} // namespace april::core
