# Implementation Plan - Enhanced Logger Formatting and Stylization

This plan covers the refactoring of the `april::Logger`, the implementation of configurable layouts, and the creation of a string stylization library.

## Phase 1: Style Refactor and Config Foundation

- [x] Task: Refactor `engine/core/source/core/log/` for April Code Style
    - [x] Audit `logger.hpp`, `logger.cpp`, `log_sink.hpp`, and `log_types.hpp`.
    - [x] Apply April code style (e.g., camelCase for methods, m_prefix for members, consistent spacing).
- [x] Task: Extend `LogConfig` and Prefix Logic
    - [x] Update `LogConfig` to include boolean flags for `showTime`, `showName`, `showLevel`, and `showThreadID`.
    - [x] Refactor `Logger::buildPrefix` to use these flags and implement the `YYYY-MM-DD HH:MM:SS` date format.
    - [x] Implement Bold ANSI styling for prefix components in `ConsoleSink`.
- [x] Task: Conductor - User Manual Verification 'Phase 1: Style Refactor and Config Foundation' (Protocol in workflow.md)

## Phase 2: Layout Optimization and Location Trace

- [x] Task: Reposition File/Line Information
    - [x] Modify `Logger::log` (or equivalent internal method) to move `[File:Line]` to the end of the message.
    - [x] Implement logic to only show `[File:Line]` if `level >= ELogLevel::Warning`.
- [x] Task: Verify Layout and Conditional Location
    - [x] Write tests to verify the `[Date Time] [Name] [Level] [ThreadID] Message [File:Line]` layout.
    - [x] Write tests to confirm File/Line info is absent for `Info` and present for `Warning`.
- [x] Task: Conductor - User Manual Verification 'Phase 2: Layout Optimization and Location Trace' (Protocol in workflow.md)

## Phase 3: Stylization Library and Format Integration

- [x] Task: Implement Log Stylization Utility
    - [x] Create `engine/core/source/core/log/log_style.hpp`.
    - [x] Implement a utility or custom `std::format` extensions to support inline styling (e.g., custom format specifiers or simple markup translation).
- [x] Task: Update `ConsoleSink` for Custom Styles
    - [x] Ensure the styling library's output is correctly handled by the `ConsoleSink` (translating markers to ANSI codes).
- [x] Task: Verification and Examples
    - [x] Create a test case demonstrating: `AP_INFO("Styled: {}", LogStyle::bold().green("Success"));` or equivalent syntax.
- [x] Task: Conductor - User Manual Verification 'Phase 3: Stylization Library and Format Integration' (Protocol in workflow.md)
