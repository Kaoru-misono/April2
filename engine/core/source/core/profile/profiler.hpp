#pragma once

#include <algorithm>
#include <cassert>
#include <limits>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <list>
#include <array>

#include "timers.hpp"

namespace april::core {

class ProfilerManager;

// The ProfilerTimeline allows to measure timed sections
// along a single timeline.
class ProfilerTimeline
{
public:
  static constexpr uint32_t MAX_FRAME_DELAY = 4;
  static constexpr uint32_t MAX_LAST_FRAMES = 128;

  struct CreateInfo
  {
    std::string name;
    uint32_t frameConfigDelay = 8;
    uint32_t defaultTimers = 128;
    uint32_t frameDelay = MAX_FRAME_DELAY;
    uint32_t frameAveragingCount = MAX_LAST_FRAMES;
  };

  struct FrameSectionID
  {
    uint32_t id : 28;
    uint32_t subFrame : 4;
  };

  struct AsyncSectionID
  {
    uint32_t id;
  };

  typedef std::function<bool(FrameSectionID, double& gpuTime)> gpuFrameTimeProvider_fn;
  typedef std::function<bool(AsyncSectionID, double& gpuTime)> gpuAsyncTimeProvider_fn;

  struct GpuTimeProvider
  {
    std::string             apiName;
    gpuFrameTimeProvider_fn frameFunction;
    gpuAsyncTimeProvider_fn asyncFunction;

    static inline uint32_t getTimerBaseIdx(FrameSectionID slot)
    {
      return ((slot.id * MAX_FRAME_DELAY) + slot.subFrame) * 2;
    }

    static inline uint32_t getTimerBaseIdx(AsyncSectionID slot) { return (slot.id * 2); }
  };

  inline void frameAdvance()
  {
    if(m_inFrame)
    {
      frameEnd();
    }
    frameBegin();
  }

  FrameSectionID frameBeginSection(const std::string& name, GpuTimeProvider* gpuTimeProvider = nullptr);
  void frameEndSection(FrameSectionID sec);
  void frameResetCpuBegin(FrameSectionID sec);
  void frameAccumulationSplit();

  AsyncSectionID asyncBeginSection(const std::string& name, GpuTimeProvider* gpuTimeProvider = nullptr);
  void asyncEndSection(AsyncSectionID sec);
  void asyncResetCpuBegin(AsyncSectionID sec);
  void asyncRemoveTimer(const std::string& name);

  struct TimerStats
  {
    double   last        = 0;
    double   average     = 0;
    double   absMinValue = 0;
    double   absMaxValue = 0;
    uint32_t index       = 0;

    std::array<double, MAX_LAST_FRAMES> times = {};
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
    size_t id;
    std::vector<TimerInfo> timerInfos;
    std::vector<std::string> timerNames;
    std::vector<std::string> timerApiNames;

    void appendToString(std::string& stats, bool full) const;
  };

  const std::string& getName() const { return m_info.name; }
  ProfilerManager*   getProfiler() const { return m_profiler; }

  void getAsyncSnapshot(Snapshot& snapShot) const;
  bool getAsyncTimerInfo(const std::string& name, TimerInfo& timerInfo, std::string& apiName) const;

  void getFrameSnapshot(Snapshot& snapShot) const;
  bool getFrameTimerInfo(const std::string& name, TimerInfo& info, std::string& apiName) const;

  void clear();
  void resetFrameSections(uint32_t delay = 0);
  void setFrameAveragingCount(uint32_t num);

  class FrameSection
  {
  public:
    FrameSection(ProfilerTimeline& ProfilerTimeline, FrameSectionID id)
        : m_ProfilerTimeline(ProfilerTimeline)
        , m_id(id) {};
    ~FrameSection() { m_ProfilerTimeline.frameEndSection(m_id); }

  private:
    ProfilerTimeline& m_ProfilerTimeline;
    FrameSectionID    m_id;
  };

  FrameSection frameSection(const std::string& name) { return FrameSection(*this, frameBeginSection(name, nullptr)); }

  class AsyncSection
  {
  public:
    AsyncSection(ProfilerTimeline& ProfilerTimeline, AsyncSectionID id)
        : m_ProfilerTimeline(ProfilerTimeline)
        , m_id(id) {};
    ~AsyncSection() { m_ProfilerTimeline.asyncEndSection(m_id); }

  private:
    ProfilerTimeline& m_ProfilerTimeline;
    AsyncSectionID    m_id;
  };

  AsyncSection asyncSection(const std::string& name) { return AsyncSection(*this, asyncBeginSection(name, nullptr)); }

  ProfilerTimeline() = default;

protected:
  friend class ProfilerManager;

  ProfilerTimeline(ProfilerManager* profiler, const ProfilerTimeline::CreateInfo& createInfo);

  void frameBegin();
  void frameEnd();

  FrameSectionID frameGetSectionID();

  bool frameGetTimerInfo(uint32_t i, TimerInfo& info);
  void frameInternalSnapshot();

  bool asyncGetTimerInfo(uint32_t i, TimerInfo& info) const;

  static constexpr uint32_t LEVEL_SINGLESHOT = ~0;

  struct TimeValues
  {
    double valueLast   = 0;
    double valueTotal  = 0;
    double absMinValue = std::numeric_limits<double>::max();
    double absMaxValue = 0;

    uint32_t cycleIndex = 0;
    uint32_t cycleCount = MAX_LAST_FRAMES;
    uint32_t validCount = 0;

    std::array<double, MAX_LAST_FRAMES> times = {};

    TimeValues(uint32_t averagedFrameCount_ = MAX_LAST_FRAMES) { init(averagedFrameCount_); }

    void init(uint32_t averagedFrameCount_);
    void reset();
    void add(double time);
    double getAveraged();
  };

  struct SectionData
  {
    std::string      name            = {};
    GpuTimeProvider* gpuTimeProvider = nullptr;

    uint32_t level    = 0;
    uint32_t subFrame = 0;

    std::array<double, MAX_FRAME_DELAY> cpuTimes = {};
    std::array<double, MAX_FRAME_DELAY> gpuTimes = {};

    uint32_t numTimes = 0;

    TimeValues gpuTime;
    TimeValues cpuTime;

    bool splitter = false;
    bool accumulated = false;
  };

  struct FrameData
  {
    uint32_t averagingCount     = MAX_LAST_FRAMES;
    uint32_t averagingCountLast = MAX_LAST_FRAMES;
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

  void grow(std::vector<SectionData>& sections, size_t newSize, uint32_t averagingCount);

  bool             m_inFrame = false;
  ProfilerManager* m_profiler{};

  ProfilerTimeline::CreateInfo m_info;

  FrameData          m_frame;
  Snapshot           m_frameSnapshot;
  mutable std::mutex m_frameSnapshotMutex;

  AsyncData          m_async;
  mutable std::mutex m_asyncMutex;
};

class ProfilerManager
{
public:
  ProfilerManager() = default;
  ~ProfilerManager();

  ProfilerTimeline* createTimeline(const ProfilerTimeline::CreateInfo& createInfo);
  void              destroyTimeline(ProfilerTimeline* timeline);

  inline double getMicroseconds() const { return m_timer.getMicroseconds(); }

  void setFrameAveragingCount(uint32_t num);
  void resetFrameSections(uint32_t delayInFrames);

  void appendPrint(std::string& statsFrames, std::string& statsAsyncs, bool full = false) const;

  void getSnapshots(std::vector<ProfilerTimeline::Snapshot>& frameSnapshots,
                    std::vector<ProfilerTimeline::Snapshot>& asyncSnapshots) const;

protected:
  std::list<std::unique_ptr<ProfilerTimeline>> m_timelines;
  mutable std::mutex                           m_mutex;
  PerformanceTimer                             m_timer;
};

class GlobalProfiler
{
public:
    static void init(const std::string& timelineName = "Main");
    static void shutdown();
    static ProfilerManager* getManager();
    static ProfilerTimeline* getTimeline(); // Main thread timeline
};

}  // namespace april::core

#define AP_PROFILE_SCOPE(name) \
    std::unique_ptr<april::core::ProfilerTimeline::FrameSection> _ap_profile_scope_##__LINE__; \
    if (auto* _tl = april::core::GlobalProfiler::getTimeline()) { \
        _ap_profile_scope_##__LINE__ = std::make_unique<april::core::ProfilerTimeline::FrameSection>(_tl->frameSection(name)); \
    }