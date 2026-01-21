# Implementation Plan - CPU/GPU Profiler

## Phase 1: Core Architecture & CPU Profiling (Core Module) [checkpoint: 55e1064]
 Establish a robust, thread-safe profiling backbone inspired by Falcor/nvpro_core2.

- [x] Task: Design and Implement `Profiler` Manager (`engine/core/source/core/profile/profiler.hpp|cpp`). [c9ba787]
    - [ ] Create `Profiler` singleton/manager.
    - [ ] Implement `ProfilerEvent` structure to hold comprehensive data (name, file, line, thread ID, CPU start/end, GPU start/end, detailed stats).
    - [ ] Implement robust Thread-Local Storage (TLS) for lock-free event capture from worker threads.
- [x] Task: Implement Advanced `CpuTimer` and Macros. [517d115]
    - [ ] Implement high-resolution `CpuTimer`.
    - [ ] Create macros `AP_PROFILE_SCOPE(name)`, `AP_PROFILE_FUNCTION()` that capture source location (`std::source_location`) for rich debugging context.
    - [ ] Test: Stress test with multiple threads to ensure data integrity and low overhead.
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Core Architecture & CPU Profiling (Core Module)' (Protocol in workflow.md)

## Phase 2: Advanced GPU Profiling (Graphics Module)
 Implement a full-featured GPU profiler in `graphics` that matches reference capabilities.

- [ ] Task: Implement `GpuProfiler` in `engine/graphics`.
    - [ ] Implement `GpuProfiler` class that manages its own GPU timestamp pools (similar to `nvpro_core2`'s `ProfilerVK`).
    - [ ] Support nested GPU markers/scopes.
    - [ ] Handle multiple command lists/queues if applicable (syncing timestamps across queues).
    - [ ] Implement a system to resolve GPU timestamps back to CPU time (calibration) for unified visualization.
    - [ ] Create a bridge/listener to feed resolved GPU events back into the `core::Profiler` for unified storage.
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Advanced GPU Profiling (Graphics Module)' (Protocol in workflow.md)

## Phase 3: Data Aggregation & Rich Export
 Support complex analysis workflows.

- [ ] Task: Implement Advanced Sliding Window & Aggregation.
    - [ ] Implement a history buffer (e.g., last 60-600 frames) to allow capturing intermittent spikes.
    - [ ] Implement frame-based aggregation (min/max/avg time per scope over the window) for statistical analysis, not just raw timeline data.
- [ ] Task: Implement Full Chrome Tracing Export.
    - [ ] Create `serializeToJson()` supporting the full Chrome Tracing event format (ph: 'X' for complete events, args for metadata).
    - [ ] Include thread names and process metadata in the export.
    - [ ] Test: Verify complex nested timelines and multi-threaded data export correctly to `chrome://tracing`.
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Data Aggregation & Rich Export' (Protocol in workflow.md)
