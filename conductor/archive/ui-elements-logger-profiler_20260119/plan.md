# Implementation Plan - Element Logger and Profiler

## Phase 1: Infrastructure and ImPlot Integration [checkpoint: 0ab9cb9]
- [x] Task: Integrate ImPlot Library [f8ce533]
    - [ ] Add ImPlot source files to `engine/core/library/implot/`
    - [ ] Update `engine/ui/CMakeLists.txt` to include and link ImPlot
    - [ ] Verify ImPlot can be initialized in a test environment
- [x] Task: Profiler Backend Development [933621f]
    - [ ] Implement `ProfilerManager` in `engine/core/source/core/profile/`
    - [ ] Implement CPU high-resolution timer support
    - [ ] Implement GPU timestamp collection via `slang-rhi` (if supported) or placeholder hooks
    - [ ] Define `AP_PROFILE_SCOPE` and related macros
- [x] Task: Conductor - User Manual Verification 'Phase 1: Infrastructure' (Protocol in workflow.md)

## Phase 2: Element Logger Implementation [checkpoint: d721e61]
- [x] Task: Element Logger Core [ade9da0]
    - [x] Create `engine/ui/source/ui/element/element-logger.hpp` and `.cpp`
    - [x] Implement `ILogSink` interface to capture `april::Logger` output
    - [x] Implement thread-safe log buffer management
- [x] Task: Element Logger UI [ade9da0]
    - [x] Implement ImGui window rendering with `onUIRender()`
    - [x] Implement color-coding for different log levels
    - [x] Add text filtering and "Clear" functionality
- [x] Task: Conductor - User Manual Verification 'Phase 2: Element Logger' (Protocol in workflow.md)

## Phase 3: Element Profiler Implementation
- [x] Task: Element Profiler UI [ade9da0]
    - [x] Create `engine/ui/source/ui/element/element-profiler.hpp` and `.cpp`
    - [x] Implement frame history buffer to store profiling data
    - [x] Implement `onUIRender()` to draw timing tables
- [x] Task: ImPlot Visualization [ade9da0]
    - [x] Implement real-time line charts for frame times using `ImPlot`
    - [x] Implement bar/pie charts for task distribution (CPU vs GPU)
- [x] Task: Conductor - User Manual Verification 'Phase 3: Element Profiler' (Protocol in workflow.md)

## Phase 4: Integration and Verification [checkpoint: 71f0d49]
- [x] Task: Testing in `test-nv-template.cpp`
    - [x] Update `engine/test/test-nv-template.cpp` to include the new elements
    - [x] Generate various log levels to verify `ElementLogger`
    - [x] Add profiled scopes to verify `ElementProfiler` data accuracy
- [x] Task: Final Polish
    - [x] Ensure all file names follow `kebab-case` and functions follow `snake_case`
    - [x] Verify no direct dependencies on `nvpro_core2` headers
- [x] Task: Conductor - User Manual Verification 'Phase 4: Integration' (Protocol in workflow.md)
