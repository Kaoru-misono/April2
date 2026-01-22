# Specification: High-Performance CPU/GPU Profiler Core

## Overview
Implement a high-performance, multi-threaded profiling system for the April Engine. This system will collect timing data from both CPU (multiple threads) and GPU (via April's RHI abstraction), maintain a sliding window of historical data, and support exporting this data to a JSON format compatible with Chrome Tracing.

## Functional Requirements
- **Multi-Threaded CPU Profiling:** Support concurrent timing events from multiple worker threads without significant lock contention.
- **April RHI Integration:** Utilize April's RHI wrappers (not raw `slang-rhi`) to collect accurate GPU timestamps.
- **RAII-Based Timing:** Provide macros or classes (e.g., `AP_PROFILE_SCOPE`) that automatically handle event start/stop using RAII, primarily influenced by Falcor's modern C++ architecture.
- **Continuous Recording:** Maintain a sliding window of profile data in memory.
- **JSON Export:** Implement an exporter that serializes the collected data into the Chrome Tracing JSON format (`chrome://tracing`).
- **Clean Slate Implementation:** The implementation must be fresh and located within the `core/profile` module structure (e.g., `engine/core/source/core/profile/`), avoiding dependencies on any existing temporary or reference files in that folder.
- **Encapsulation:** All profiling logic must reside within the `april` namespace and follow the project's C++23 style guide.

## Non-Functional Requirements
- **Performance:** Minimizing the overhead of profiling calls is critical. Use efficient storage (e.g., lock-free queues or per-thread buffers).
- **Style:** Adhere to `conductor/code_styleguides/general.md`.

## Acceptance Criteria
- A `Profiler` class exists and manages the session.
- `AP_PROFILE_SCOPE` (or similar) macro captures CPU duration.
- GPU profiling captures execution time using April RHI timestamps.
- Data export produces a valid JSON file readable by Chrome Tracing.
- Unit tests verify the recording mechanics and data integrity.

## Out of Scope
- **UI Implementation:** The visualization widget is deferred.
