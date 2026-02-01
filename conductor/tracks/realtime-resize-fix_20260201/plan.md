# Implementation Plan: Real-Time Window Resize Support

This plan covers the refactoring of the `Engine` class to support continuous rendering during window resize operations, following the pattern verified in `test-swapchain-resize.cpp`.

## Phase 1: Engine Logic Refactoring [checkpoint: bf92cdb]
Refactor the `Engine::run` loop to extract frame rendering into a reusable method.

- [x] Task: Refactor `Engine` class to extract `renderFrame` method.
    - [x] Add `auto renderFrame(float deltaTime) -> void;` to private section of `Engine` in `engine.hpp`.
    - [x] Move the contents of the `while (m_running)` loop from `engine.cpp` into `renderFrame`.
    - [x] Update `Engine::run` to call `renderFrame` in its loop.
- [x] Task: Implement Immediate Resize Logic.
    - [x] Update `FrameBufferResizeEvent` subscription in `Engine::init`.
    - [x] In the callback, if width/height > 0:
        - [x] Manually call `m_swapchain->resize` and `ensureOffscreenTarget`.
        - [x] Call `renderFrame(0.0f)` to force a refresh.
        - [x] Clear `m_swapchainDirty` flag.
- [x] Task: Conductor - User Manual Verification 'Phase 1: Engine Logic Refactoring' (Protocol in workflow.md)

## Phase 2: Verification and Cleanup
Verify the fix in the Editor and ensure no regressions.

- [x] Task: Verify fix in Editor.
    - [x] Build and run `April_editor`.
    - [x] Perform window resizing and confirm real-time updates.
- [x] Task: Conductor - User Manual Verification 'Phase 2: Verification and Cleanup' (Protocol in workflow.md)
