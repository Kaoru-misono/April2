# Specification: Real-Time Window Resize Support

## 1. Overview
Currently, when the engine window is resized or dragged on Windows, rendering pauses until the mouse button is released. This is because `glfwPollEvents` enters a blocking modal loop on Windows. This feature aims to implement real-time rendering during window resize operations by intercepting the resize callback and triggering an immediate frame update.

## 2. Functional Requirements
- **Real-Time Update:** The engine MUST continue to render frames while the user is resizing the window.
- **Immediate Resize:** The swapchain MUST be resized immediately when the window size changes, rather than waiting for the next main loop iteration.
- **Visual Continuity:** There should be no black frames or artifacts during the resize operation.

## 3. Technical Implementation
- **Refactor `Engine::run`:** Extract the frame rendering logic (update, render, UI, present) into a new private method `Engine::renderFrame(float dt)`.
- **Event Handling:** Update the `FrameBufferResizeEvent` subscription in `Engine::init()` to:
    1.  Immediately resize the Swapchain and Offscreen targets.
    2.  Call `renderFrame(0.0f)` (or a small delta) to force an immediate update and present.
- **Thread Safety:** Ensure `vkDeviceWaitIdle` is called (via `Swapchain::resize`) to safely recreate resources while the GPU might be busy.

## 4. Acceptance Criteria
- [ ] Launch the Editor or Game entry point.
- [ ] Resize the window by dragging the borders.
- [ ] Verify that the 3D scene and UI update continuously while dragging, without pausing.
- [ ] Verify that no validation layer errors occur during the resize.

## 5. Out of Scope
- Multi-threaded rendering optimizations (this fix assumes the current single-threaded architecture).
