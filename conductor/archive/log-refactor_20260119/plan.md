# Implementation Plan - Refactor Core Logging System

This plan covers the refactoring of the existing logging system in `engine/core`. We will reuse existing logic (e.g., existing severity enums or file handling) where possible, while upgrading the architecture to support thread safety, multiple sinks, `std::format`, and `std::source_location`.

## Phase 1: Analysis and Foundation [checkpoint: fad5f46]

- [x] Task: Analyze current `engine/core` logger implementation and `nvpro_core2` reference
    - [x] Read `engine/core/source/core/log/logger.hpp` and `logger.cpp` to identify reusable parts (e.g., enums, existing file logic)
    - [x] Read `nvpro_core2/nvutils/logger.hpp` to understand reference patterns for thread safety
- [x] Task: Define the new Log Sink interface and Severity levels
    - [x] Reuse or extend existing `LogLevel` enum if compatible
    - [x] Define `ILogSink` abstract base class
- [x] Task: Conductor - User Manual Verification 'Phase 1: Analysis and Foundation' (Protocol in workflow.md)

## Phase 2: Core Implementation (TDD) [checkpoint: f8bfe91]

- [x] Task: Refactor Logger Core for Thread Safety
    - [x] Write failing tests for thread-safe logging
    - [x] Refactor `Logger` class to use a mutex-protected sink list (reuse existing singleton/instance logic if present)
    - [x] Update internal formatting logic to use `std::format`
    - [x] Integrate `std::source_location` for automatic context capturing
    - [x] Verify tests pass
- [x] Task: Implement Standard Sinks
    - [x] Write failing tests for Console and File sinks
    - [x] Refactor existing console output logic into a `ConsoleSink`
    - [x] Refactor/Add `FileSink` for disk logging (reuse file I/O if present)
    - [x] Add `DebugSink` (OutputDebugString for Windows)
    - [x] Verify tests pass
- [x] Task: Conductor - User Manual Verification 'Phase 2: Core Implementation (TDD)' (Protocol in workflow.md)

## Phase 3: Integration and Refinement [checkpoint: f29f801]

- [x] Task: Update Logger Macros
    - [x] Refactor existing macros to use the new `std::source_location` enabled API
- [x] Task: Replace old usage and verify
    - [x] Ensure the engine still compiles and logs correctly using the new system
- [x] Task: Conductor - User Manual Verification 'Phase 3: Integration and Refinement' (Protocol in workflow.md)
