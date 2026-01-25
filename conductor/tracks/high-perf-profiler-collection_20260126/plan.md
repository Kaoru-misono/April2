# Implementation Plan - Profiler Phase 2: High-Performance Data Collection

## Phase 1: Event Structure and Thread-Local Buffers
- [x] Task: Define `ProfileEvent` and `ProfileBuffer`
    - [x] Update `engine/core/source/core/profile/profiler.hpp` with the `ProfileEvent` struct (alignas(32)).
    - [x] Implement `ProfileBuffer` class to manage thread-local pre-allocated event storage.
- [x] Task: Implement TLS Integration
    - [x] Create `thread_local` instance of `ProfileBuffer` in `profiler.cpp`.
    - [x] Update `Profiler::startEvent` and `Profiler::endEvent` to write directly to the local buffer.
- [x] Task: Conductor - User Manual Verification 'Phase 1: Event Structure and Thread-Local Buffers' (Protocol in workflow.md)

## Phase 2: ProfileManager and Global Aggregation
- [x] Task: Implement `ProfileManager` Singleton
    - [x] Create `engine/core/source/core/profile/profile-manager.hpp` and `profile-manager.cpp`.
    - [x] Implement thread registration logic (registering TLS buffers).
    - [x] Implement `flush()` to collect data from all registered buffers.
- [x] Task: Integrate Profiler with Manager
    - [x] Ensure `Profiler singleton notifies `ProfileManager` of new thread buffers.
    - [x] Handle thread-safe aggregation during the flush cycle.
- [x] Task: Conductor - User Manual Verification 'Phase 2: ProfileManager and Global Aggregation' (Protocol in workflow.md)

## Phase 3: Testing and Performance Verification
- [x] Task: Multi-threaded Unit Testing
    - [x] Update `engine/test/test-profiler.cpp` with multi-threaded scenarios.
    - [x] Verify that events from different threads are correctly collected and ordered by timestamp.
- [x] Task: Performance Benchmarking
    - [x] Write a benchmark test to measure the cycles/nanoseconds per profiling event.
    - [x] Ensure the overhead is within acceptable limits (sub-100ns).
- [x] Task: Conductor - User Manual Verification 'Phase 3: Testing and Performance Verification' (Protocol in workflow.md)
