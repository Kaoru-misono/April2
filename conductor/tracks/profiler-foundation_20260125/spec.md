# Specification - Profiler Phase 1: Foundational Instrumentation

## 1. Overview
This track implements the foundational layer of the April Engine profiler. It provides a high-precision `Timer` and a `Profiler` management class to handle event instrumentation. This design allows for both CPU and future GPU profiling by abstracting the timing source through the `Profiler` interface.

## 2. Functional Requirements

### 2.1 Core Timing Source (Timer)
- **Namespace:** `april::core`
- **Location:** `engine/core/source/core/profile/timer.hpp` (and `.cpp` if needed).
- **Implementation:** Use `std::chrono::high_resolution_clock`.
- **API:**
    - `static auto now() -> TimePoint`: Returns the current high-precision timestamp.
    - `static auto calcDuration(TimePoint start, TimePoint end) -> double`: Returns the duration in milliseconds.

### 2.2 Profiler Management (Profiler)
- **Namespace:** `april::core`
- **Location:** `engine/core/source/core/profile/profiler.hpp` (and `.cpp`).
- **Role:** Acts as the central hub for instrumentation. In this phase, it will manage the "active" status of events but perform **no-op** recording.
- **API:**
    - `void startEvent(const std::string& name)`: Marks the start of a profiling event.
    - `void endEvent(const std::string& name)`: Marks the end of a profiling event.
    - (Future-proofing) Overloads or parameters to accept GPU/Render contexts.

### 2.3 RAII Instrumentation (ScopedProfileZone)
- **RAII Class:** `ScopedProfileZone` in `april::core`.
- **Logic:** Calls `Profiler::get().startEvent()` in the constructor and `Profiler::get().endEvent()` in the destructor.
- **Macro:** `APRIL_PROFILE_ZONE`
    - Automatically captures the function name.
    - Supports an optional string for custom naming: `APRIL_PROFILE_ZONE()` or `APRIL_PROFILE_ZONE("MyZone")`.

## 3. Technical Strategy
- Follow the **NVIDIA Falcor** pattern where a central `Profiler` manages events, making it easy to add GPU timestamp injection later without changing every instrumentation site.
- Use `april::core::ref` and engine-standard naming (trailing return types, etc.).

## 4. Acceptance Criteria
- [ ] `Timer` class measures time correctly using `std::chrono`.
- [ ] `Profiler` singleton successfully receives `start`/`end` signals.
- [ ] `APRIL_PROFILE_ZONE` macro correctly instruments code blocks.
- [ ] Unit tests verify the `Timer` accuracy and RAII lifecycle.
