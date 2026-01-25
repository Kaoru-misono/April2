# Implementation Plan - Refactor ResourceBindFlags

## Phase 1: Core Utilities and Timing Source [checkpoint: e5d29ef]
- [x] Task: Implement `april::core::Timer`
    - [x] Create `engine/core/source/core/profile/timer.hpp` using `std::chrono::high_resolution_clock`.
    - [x] Implement `now()`, `calcDuration()`, and convenience conversion methods (ms, us, ns).
- [x] Task: Create `april::core::Profiler` Foundation
    - [x] Create `engine/core/source/core/profile/profiler.hpp` and `profiler.cpp`.
    - [x] Implement a singleton `Profiler` class with `startEvent()` and `endEvent()` methods (no-op recording for now).
- [x] Task: Implement `ScopedProfileZone` and Macros
    - [x] Create `ScopedProfileZone` RAII class in `profiler.hpp` that communicates with the `Profiler` singleton.
    - [x] Define `APRIL_PROFILE_ZONE` macros with hybrid naming (automatic function capture + optional custom name).
- [x] Task: Conductor - User Manual Verification 'Phase 1: Core Utilities and Timing Source' (Protocol in workflow.md)

## Phase 2: Testing and Verification [checkpoint: 9242ebd]
- [x] Task: Unit Testing for Timing and RAII
    - [x] Create `engine/test/test-profiler.cpp`.
    - [x] Write tests to verify `Timer` precision and duration calculations.
    - [x] Write tests to verify `ScopedProfileZone` correctly triggers `startEvent` and `endEvent` on the singleton.
- [x] Task: Build and Execution Verification
    - [x] Build the engine in Debug mode.
    - [x] Execute the unit tests and ensure they pass.
- [x] Task: Conductor - User Manual Verification 'Phase 2: Testing and Verification' (Protocol in workflow.md)
