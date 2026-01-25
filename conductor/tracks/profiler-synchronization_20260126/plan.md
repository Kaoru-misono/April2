# Implementation Plan - Profiler Phase 4: Synchronization

## Phase 1: Enhanced Calibration Mechanism
- [ ] Task: Implement Calibration Storage
    - [ ] Update `GpuProfiler::FrameData` to store the CPU reference time (`cpuReferenceNs`) captured during `submit()`.
    - [ ] Ensure `GpuProfiler` records a calibration timestamp at the very beginning of every frame.
- [ ] Task: Implement Calibration Capture
    - [ ] Add `GpuProfiler::beginFrameCalibration(CommandContext* pContext)` to record the initial GPU timestamp.
    - [ ] Add `GpuProfiler::endFrameCalibration()` to be called during `Device::endFrame` (before/after submit) to capture and average CPU time.
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Enhanced Calibration Mechanism' (Protocol in workflow.md)

## Phase 2: Synchronization and Mapping
- [ ] Task: Implement Linear Mapping with Rolling Average
    - [ ] Add rolling average logic to filter the `cpu_base - (gpu_base * frequency)` offset.
    - [ ] Update `GpuProfiler` processing logic to apply the mapping formula using the *matching* frame's CPU reference data.
- [ ] Task: Update `collectEvents` Output
    - [ ] Verify `ProfileEvent::timestamp` uses the unified CPU timeline (nanoseconds).
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Synchronization and Mapping' (Protocol in workflow.md)

## Phase 3: Verification
- [ ] Task: Timing Alignment Test
    - [ ] Update `engine/test/test-gpu-profiler.cpp` to verify that GPU events appear strictly after the CPU `submit()` call but before `endFrame()`.
- [ ] Task: Conductor - User Manual Verification 'Phase 3: Verification' (Protocol in workflow.md)