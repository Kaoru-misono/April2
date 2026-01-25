# Specification - Profiler Phase 4: Synchronization

## 1. Overview
This track implements the synchronization logic required to align GPU and CPU timelines without modifying the underlying RHI layer. It uses a best-effort calibration strategy to correlate GPU timestamps with the high-precision CPU clock, filtering for jitter using a rolling average.

## 2. Functional Requirements

### 2.1 Refined Calibration Capture
- **Mechanism:** Capture `april::core::Timer::now()` immediately before and after calling `CommandContext::submit()` for the primary command buffer.
- **Tracking:** The averaged CPU timestamp from Frame $N$ must be stored in that frame's `FrameData` to correctly match with readback results from Frame $N$ (compensating for $N+2$ latency).
- **Command Sequence:** Record a "Calibration" GPU timestamp as the very first command in the frame's primary command buffer.

### 2.2 Linear Mapping & Filtering
- **Rolling Average:** Maintain a moving average of the time offset between CPU and GPU clocks to filter out OS-level jitter during `submit()`.
- **Formula:** 
  `aligned_timestamp_ns = cpu_base_ns + (gpu_ticks - gpu_base_ticks) * ns_per_tick`
- **EAST Const & Style:** Maintain `m_` prefix and East Const style for internal state.

### 2.3 Automatic Conversion
- **Integration:** Update `GpuProfiler::endFrame` to use the mapping formula when processing resolved timestamps.
- **Output:** Ensure `collectEvents()` returns `ProfileEvent` objects aligned to the CPU timeline.

## 3. Technical Strategy
- Modify `GpuProfiler::FrameData` to include `cpuReferenceNs`.
- Implement offset filtering in `GpuProfiler`.
- Ensure strict temporal matching between calibration data and readback results.

## 4. Acceptance Criteria
- [ ] GPU events are correctly positioned relative to the CPU frames that triggered them.
- [ ] Rolling average filters out anomalous spikes in `submit()` duration.
- [ ] Timestamps are monotonic and stable across multiple frames.
- [ ] EAST Const and project coding standards are strictly followed.

## 5. Out of Scope
- Direct use of platform-specific calibration APIs (kept generic via RHI wrappers).