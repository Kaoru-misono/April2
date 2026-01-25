# Specification - Profiler Phase 3: GPU Profiling

## 1. Overview
This track implements GPU profiling support while maintaining strict architectural separation between the `core` and `graphics` modules. It introduces a registration mechanism where the `graphics` module can inject GPU event handlers into the core `Profiler`.

## 2. Functional Requirements

### 2.1 Decoupled Registration
- **Core Interface:** Define a `IGpuProfiler` interface in `april::core` (within `profiler.hpp`).
- **Registration:** `april::core::Profiler` will provide a method to register a `IGpuProfiler`.
- **Graphics Implementation:** `GpuProfiler` (in the `graphics` module) will implement this interface and register itself with the core `Profiler` upon initialization.

### 2.2 GpuProfiler Implementation
- **Management:** Owned by `graphics::Device`.
- **Query Heap:** Uses the single `QueryHeap` (Timestamp type) provided by `Device::getTimestampQueryHeap()`.
- **Asynchronous Readback:** 
    - Uses `Fence` and an N+2 buffering strategy to read back timestamps.
    - Resolves queries to a GPU-to-CPU readback buffer.
- **No-Stall Guarantee:** Data is only retrieved when the GPU completion fence is signaled.

### 2.3 Instrumentation (APRIL_GPU_ZONE)
- **Macro:** `APRIL_GPU_ZONE(pContext, "Name")`.
- **Logic:**
    - Records start/end timestamps via `pContext->writeTimestamp`.
    - Dispatches metadata to the `GpuProfiler` which is then exposed via the core interface.

### 2.4 Calibration
- Syncs GPU frequency and CPU `Timer` values to provide a unified timeline.

## 3. Technical Strategy
- **Core Change:** Add `registerGpuProfiler` and metadata collection to `april::core::Profiler`.
- **Graphics Change:** Implement `GpuProfiler` inheriting from the core provider interface.
- **Data Flow:**
    1. `APRIL_GPU_ZONE` records indices.
    2. `GpuProfiler` manages indices and readback.
    3. `ProfileManager::flush()` aggregates both CPU events and retrieved GPU events from the registered provider.

## 4. Acceptance Criteria
- [ ] `core` module compiles without any reference to `graphics` headers.
- [ ] `GpuProfiler` successfully registers with `Profiler`.
- [ ] `APRIL_GPU_ZONE` correctly measures GPU time.
- [ ] No pipeline stalls during data aggregation.

## 5. Out of Scope
- Support for non-RHI timing sources.
