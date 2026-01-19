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

## Phase 2: Element Logger Implementation
- [ ] Task: Element Logger Core
    - [ ] Create `engine/ui/source/ui/element/element-logger.hpp` and `.cpp`
    - [ ] Implement `ILogSink` interface to capture `april::Logger` output
    - [ ] Implement thread-safe log buffer management
- [ ] Task: Element Logger UI
    - [ ] Implement ImGui window rendering with `onUIRender()`
    - [ ] Implement color-coding for different log levels
    - [ ] Add text filtering and "Clear" functionality
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Element Logger' (Protocol in workflow.md)

## Phase 3: Element Profiler Implementation
- [ ] Task: Element Profiler UI
    - [ ] Create `engine/ui/source/ui/element/element-profiler.hpp` and `.cpp`
    - [ ] Implement frame history buffer to store profiling data
    - [ ] Implement `onUIRender()` to draw timing tables
- [ ] Task: ImPlot Visualization
    - [ ] Implement real-time line charts for frame times using `ImPlot`
    - [ ] Implement bar/pie charts for task distribution (CPU vs GPU)
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Element Profiler' (Protocol in workflow.md)

## Phase 4: Integration and Verification
- [ ] Task: Testing in `test-nv-template.cpp`
    - [ ] Update `engine/test/test-nv-template.cpp` to include the new elements
    - [ ] Generate various log levels to verify `ElementLogger`
    - [ ] Add profiled scopes to verify `ElementProfiler` data accuracy
- [ ] Task: Final Polish
    - [ ] Ensure all file names follow `kebab-case` and functions follow `snake_case`
    - [ ] Verify no direct dependencies on `nvpro_core2` headers
- [ ] Task: Conductor - User Manual Verification 'Phase 4: Integration' (Protocol in workflow.md)
