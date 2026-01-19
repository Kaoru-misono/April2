# Specification: Element Logger and Profiler Implementation

## Overview
Implement two new UI elements, `ElementLogger` and `ElementProfiler`, for the April engine's UI module. These elements will provide runtime debugging and performance monitoring capabilities similar to those found in `nvpro_core2`.

## Functional Requirements

### 1. ElementLogger
- **Implementation:** Located in `engine/ui/source/ui/element/element-logger.hpp/cpp`.
- **Inheritance:** Must implement `april::ui::IElement` and `april::ILogSink`.
- **Log Interception:** Register itself as a sink to the global `april::Logger` on attachment.
- **UI Features:**
  - Dedicated ImGui window for log display.
  - Level filtering (Trace, Debug, Info, Warning, Error, Fatal).
  - Auto-scrolling and text filtering (search).
  - Color-coded log entries based on severity.

### 2. ElementProfiler
- **Implementation:** Located in `engine/ui/source/ui/element/element-profiler.hpp/cpp`.
- **Inheritance:** Must implement `april::ui::IElement`.
- **Profiling Backend:** 
  - Implementation of a `ProfilerManager` in `engine/core/source/core/profile/` to track CPU and GPU timestamps.
  - Support for RAII scoped timers (`AP_PROFILE_SCOPE`).
- **UI Features:**
  - Integration of `ImPlot` for graphical visualization.
  - Frame timing graphs (Line charts).
  - Breakdown of CPU/GPU tasks (Bar/Pie charts).
  - Hierarchical table view of timing results.

### 3. Library Integration
- **ImPlot:** Integrate `ImPlot` into the `engine/ui` module.
- **Dependency Management:** Ensure proper CMake linking for `ImPlot`.

## Non-Functional Requirements
- **Performance:** Profiling and logging should have minimal overhead when the UI is closed.
- **Coding Style:** Adhere to April's `snake_case` for functions/variables and `kebab-case` for filenames.
- **C++ Standard:** Utilize C++23 features (Modules, Concepts) where appropriate.

## Acceptance Criteria
- [ ] `ElementLogger` correctly displays all logs from `AP_INFO`, `AP_ERROR`, etc.
- [ ] `ElementProfiler` displays real-time CPU/GPU frame times.
- [ ] `ImPlot` graphs render correctly within the profiler window.
- [ ] Both elements are tested in `engine/test/test-nv-template.cpp`.
- [ ] No direct linkage to `nvpro_core2` (learned and referenced only).

## Out of Scope
- Advanced GPU hardware metrics (NVML) – unless already provided by `slang-rhi`.
- Networked logging/profiling.
