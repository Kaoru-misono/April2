# Specification: Enhanced Logger Formatting and Stylization

## 1. Overview
This track focuses on refining the existing `april::Logger` to adhere strictly to the project's code style, implement highly configurable log prefixes, and introduce a powerful string stylization system. The goal is to improve log readability and provide developers with fine-grained control over log appearance.

## 2. Functional Requirements

### 2.1 Code Style & Formatting
- **Code Refactoring:** Refactor all files in `engine/core/source/core/log/` to strictly follow the April Engine code style (indentation, naming conventions, and file structure observed in `engine/core` and `engine/graphics`).
- **Date/Time Format:** Update the timestamp format to `YYYY-MM-DD HH:MM:SS`.

### 2.2 Configurable Log Layout
- **Prefix Configuration:** Extend `LogConfig` to allow runtime enabling/disabling of prefix components: `Date/Time`, `Name`, `Level`, and `ThreadID`.
- **Layout Structure:** The standard log layout will be: `[Date Time] [Name] [Level] [ThreadID] Message [File:Line]`.
- **Bold Styling:** In the console output, the prefix components (`[Date Time]`, `[Name]`, `[Level]`, `[ThreadID]`) must be displayed in **Bold**.
- **Location Trace Placement:** Move File and Line information to the **end** of the log message.
- **Conditional Location:** By default, File and Line information (`[File:Line]`) should only be appended for log levels greater than `Info` (i.e., `Warning`, `Error`, `Fatal`).

### 2.3 Stylized Log Strings
- **Styling Library:** Provide a utility/library to generate styled strings that can be passed to the logger.
- **Integration with std::format:** Extend or complement `std::format` to support custom style markers (e.g., custom formatters or a light markup within the format string) to allow inline styling like green bold text followed by white italic text.
- **Console Rendering:** Ensure these styles are correctly translated to ANSI escape codes in the `ConsoleSink`.

## 3. Non-Functional Requirements
- **Maintainability:** Ensure the refactored code is clean, modular, and well-documented.
- **Performance:** Log formatting and styling should have negligible impact on the engine's performance.

## 4. Acceptance Criteria
- [ ] `engine/core/source/core/log/` code passes style check.
- [ ] Logs show correct `YYYY-MM-DD HH:MM:SS` date format.
- [ ] Log prefix components are configurable via `LogConfig`.
- [ ] Log layout matches `[Date Time] [Name] [Level] [ThreadID] Message [File:Line]`.
- [ ] File/Line info is correctly placed at the end and only shown for `Warning+` by default.
- [ ] Custom styling library implemented and verified with examples (e.g., green/bold text).
