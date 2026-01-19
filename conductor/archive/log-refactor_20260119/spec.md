# Specification: Refactor Core Logging System

## 1. Overview
This track focuses on refactoring and improving the existing logging system located in `engine/core/source/core/log/`. The goal is to modernize the current implementation to be robust, thread-safe, and feature-rich, using `nvpro_core2` only as a conceptual reference.

## 2. Functional Requirements

### 2.1 Core Logging Mechanics
- **Thread Safety:** Upgrade the existing `Logger` to support concurrent calls from multiple threads (e.g., using mutexes) to prevent race conditions.
- **Severity Levels:** Ensure standard severity levels are supported: `Trace`, `Debug`, `Info`, `Warning`, `Error`, `Fatal`.
- **Modern Formatting:** Refactor internal formatting to use C++20/23 `std::format`, replacing any legacy stream or `printf` style implementations.
- **Source Context:** Use `std::source_location` (C++20) to capture file and line information automatically, reducing reliance on legacy macros where possible.

### 2.2 Sinks (Output Destinations)
- **Multi-Sink Architecture:** Refactor the logger to support multiple sinks (e.g., Console, File, IDE Debug) simultaneously.
- **Console Sink:** Ensure robust output to stdout/stderr with color support.
- **File Sink:** Add or improve support for logging to a file.
- **Debug Sink:** Add support for IDE debug output (e.g., `OutputDebugString`).

## 3. Non-Functional Requirements
- **Location:** Modifications must remain strictly within `engine/core/source/core/log/`.
- **Independence:** The code must remain independent of `nvpro_core2`.
- **Performance:** Ensure minimal overhead when logging is disabled or filtered out.

## 4. Acceptance Criteria
- [ ] **Unit Tests:**
    - Verify thread safety and multiple sink support.
    - Verify `std::source_location` correctly captures call sites.
- [ ] **Refactoring:**
    - `logger.hpp` and `logger.cpp` in `engine/core` are updated to the new architecture.
