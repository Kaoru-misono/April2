# Implementation Plan - Core Profile and UI Element Reformatting

## Phase 1: Preparation
- [x] Task: Review existing tests for Profile and UI elements.
    - [x] Run `test-profiler` and `test-ui-font` (or equivalent) to ensure baseline is passing.
    - [x] Analyze `engine/core/source/core/profile/profiler.cpp` for current style.
    - [x] Analyze `engine/ui/source/ui/element/element-profiler.cpp` for current style.
- [x] Task: Conductor - User Manual Verification 'Preparation' (Protocol in workflow.md)

## Phase 2: Core Profile Refactoring
- [x] Task: Reformat `engine/core/source/core/profile/profiler.hpp`.
    - [x] Update function declarations to trailing return type.
    - [x] Apply East Const syntax.
    - [x] Verify compilation.
- [x] Task: Reformat `engine/core/source/core/profile/profiler.cpp`.
    - [x] Update function signatures to trailing return type.
    - [x] Apply East Const syntax.
    - [x] Apply AAA (Almost Always Auto) for local variables.
    - [x] Verify compilation.
- [x] Task: Reformat `engine/core/source/core/profile/timers.hpp` and `timers.cpp`.
    - [x] Apply same rules (trailing return, East Const, AAA).
    - [x] Verify compilation.
- [x] Task: Verify Core Profile changes with tests.
    - [x] Run `test-profiler` to ensure no regressions.
- [x] Task: Conductor - User Manual Verification 'Core Profile Refactoring' (Protocol in workflow.md)

## Phase 3: UI Element Refactoring
- [x] Task: Reformat `engine/ui/source/ui/element/element-profiler.hpp`.
    - [x] Update function declarations to trailing return type.
    - [x] Apply East Const syntax.
    - [x] Verify compilation.
- [x] Task: Reformat `engine/ui/source/ui/element/element-profiler.cpp`.
    - [x] Update function signatures to trailing return type.
    - [x] Apply East Const syntax.
    - [x] Apply AAA for local variables.
    - [x] Verify compilation.
- [x] Task: Reformat `engine/ui/source/ui/element/element-logger.hpp` and `element-logger.cpp`.
    - [x] Apply same rules (trailing return, East Const, AAA).
    - [x] Verify compilation.
- [x] Task: Verify UI Element changes with tests.
    - [x] Run relevant UI tests (or build example app if no direct unit test exists).
- [x] Task: Conductor - User Manual Verification 'UI Element Refactoring' (Protocol in workflow.md)

## Phase 4: Test Suite Refactoring
- [x] Task: Reformat `engine/test/test-profiler.cpp`.
    - [x] Apply trailing return types and East Const to test cases.
    - [x] Apply AAA to test logic.
- [x] Task: Reformat other relevant tests in `engine/test/` (e.g., `test-ui-font.cpp` if touched by dependency).
- [x] Task: Final Verification.
    - [x] Run full test suite.
- [x] Task: Conductor - User Manual Verification 'Test Suite Refactoring' (Protocol in workflow.md)