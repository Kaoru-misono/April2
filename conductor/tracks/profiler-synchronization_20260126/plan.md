# Implementation Plan - Profiler Phase 4: Synchronization

## Phase 1: Enhanced Calibration Mechanism
- [x] Task: Implement Calibration Storage [sha: dbba1a3]
    - [x] Update `GpuProfiler::FrameData` to store the CPU reference time (`cpuReferenceNs`) captured during `submit()`.
    - [x] Ensure `GpuProfiler` records a calibration timestamp at the very beginning of every frame.
- [x] Task: Implement Calibration Capture [sha: dbba1a3]
    - [x] Add `GpuProfiler::beginFrameCalibration(CommandContext* pContext)` to record the initial GPU timestamp.
    - [x] Add `GpuProfiler::endFrameCalibration()` to be called during `Device::endFrame` (before/after submit) to capture and average CPU time.
- [x] Task: Conductor - User Manual Verification 'Phase 1: Enhanced Calibration Mechanism' (Protocol in workflow.md) [sha: dbba1a3]

## Phase 2: Synchronization and Mapping
- [x] Task: Implement Linear Mapping with Rolling Average [sha: dbba1a3]
    - [x] Add rolling average logic to filter the `cpu_base - (gpu_base * frequency)` offset.
    - [x] Update `GpuProfiler` processing logic to apply the mapping formula using the *matching* frame's CPU reference data.
- [x] Task: Update `collectEvents` Output [sha: dbba1a3]
    - [x] Verify `ProfileEvent::timestamp` uses the unified CPU timeline (nanoseconds).
- [x] Task: Conductor - User Manual Verification 'Phase 2: Synchronization and Mapping' (Protocol in workflow.md) [sha: dbba1a3]

## Phase 3: Verification
- [x] Task: Timing Alignment Test [sha: dbba1a3]
    - [x] Update `engine/test/test-gpu-profiler.cpp` to verify that GPU events appear strictly after the CPU `submit()` call but before `endFrame()`.
- [x] Task: Conductor - User Manual Verification 'Phase 3: Verification' (Protocol in workflow.md) [sha: dbba1a3]