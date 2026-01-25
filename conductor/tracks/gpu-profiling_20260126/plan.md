# Implementation Plan - Profiler Phase 3: GPU Profiling

## Phase 1: Core Decoupling and Registration
- [x] Task: Define `GpuProfilerInterface` in Core
    - [x] Update `engine/core/source/core/profile/profiler.hpp` to include `IGpuProfiler` interface.
    - [x] Add `registerGpuProfiler(IGpuProfiler* pGpuProfiler)` to `april::core::Profiler`.
- [x] Task: Update `ProfileManager` for GPU Data
    - [x] Update `ProfileManager::flush` to also pull and aggregate data from the registered `IGpuProfiler`.
- [x] Task: Conductor - User Manual Verification 'Phase 1: Core Decoupling and Registration' (Protocol in workflow.md)

## Phase 2: GpuProfiler Implementation (Graphics)
- [x] Task: Implement `GpuProfiler` Class
    - [x] Create `engine/graphics/source/graphics/profile/gpu-profiler.hpp` and `.cpp`.
    - [x] Implement `IGpuProfiler` interface.
    - [x] Implement query index allocation/deallocation from the `Device` timestamp heap.
- [x] Task: Implement Asynchronous Readback Strategy
    - [x] Create a readback buffer for resolving timestamps.
    - [x] Implement the N+2 fence-based tracking logic to avoid stalls.
    - [x] Add `resolveQueries` and `readbackResults` methods.
- [x] Task: Conductor - User Manual Verification 'Phase 2: GpuProfiler Implementation' (Protocol in workflow.md)

## Phase 3: Instrumentation and Integration
- [x] Task: Define `APRIL_GPU_ZONE` Macro
    - [x] Update `engine/graphics/source/graphics/profile/gpu-profiler.hpp` with the macro.
    - [x] Macro should use `CommandContext` to inject timestamps and record metadata in `GpuProfiler`.
- [x] Task: Register GpuProfiler in `Device`
    - [x] Update `graphics::Device` constructor to initialize `GpuProfiler` and register it with the core `Profiler`.
- [x] Task: GPU/CPU Calibration
    - [x] Implement logic to correlate GPU ticks with `april::core::Timer` nanoseconds.
- [x] Task: Conductor - User Manual Verification 'Phase 3: Instrumentation and Integration' (Protocol in workflow.md)

## Phase 4: Testing and Verification
- [x] Task: Create GPU Profiling Test
    - [x] Create `engine/test/test-gpu-profiler.cpp`.
    - [x] Write a test that performs a heavy GPU task (e.g., large blit or clear) and verifies non-zero GPU duration.
    - [x] Verify that no stalls occur (check CPU execution time remains consistent).
- [x] Task: Conductor - User Manual Verification 'Phase 4: Testing and Verification' (Protocol in workflow.md)
