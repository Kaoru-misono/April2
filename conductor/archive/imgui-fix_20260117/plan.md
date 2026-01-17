# Implementation Plan: Fix ImGuiLayer Scissor and Window Events

This plan addresses scissor clipping inaccuracies and window event instability (resize/DPI) in the `ImGuiLayer` implementation.

## Phase 1: Audit and Test Foundation
Identify discrepancies and establish a baseline for verification.

- [x] **Task: Comparative Audit of `ImGuiLayer`** [commit: 8583632]
    - [x] Compare `engine/graphics/source/graphics/ui/imgui-layer.cpp` with the reference `imgui_impl_vulkan.cpp`.
    - [x] Audit how `DisplaySize`, `FramebufferScale`, and `ClipRect` are handled in the current implementation.
- [x] **Task: Create Scissor and Resize Test Suite** [commit: 8583632]
    - [x] Write a unit test in `engine/test/test-imgui.cpp` (or a new file) that simulates various window sizes and DPI scales.
    - [x] Implement a test case that verifies the `setScissor` call receives coordinates correctly scaled for the framebuffer.
- [x] **Task: Conductor - User Manual Verification 'Audit'** [commit: 8583632]

## Phase 2: Fix Scissor and Clipping Logic
Ensure draw commands are correctly clipped to physical pixels.

- [x] **Task: Implement Coordinate Scaling in `ImGuiLayer`** [commit: 8583632]
    - [x] Update the vertex shader or the root constants in `ImGuiLayer::renderDrawData` to use logical coordinates.
    - [x] Update `renderDrawData` to scale `ClipRect` values by `drawData->FramebufferScale` before passing them to the RHI `setScissor` method.
- [x] **Task: Verify Clipping Fix** [commit: 8583632]
    - [x] Run the test suite and confirm that `setScissor` coordinates match the expected physical pixel values.
- [x] **Task: Conductor - User Manual Verification 'Scissor Fix'** [commit: 8583632]

## Phase 3: Stabilize Window and DPI Events
Prevent UI loss and flickering during state transitions.

- [x] **Task: Fix Resize and DPI Event Propagation** [commit: 8583632]
    - [x] Audit `Application::onViewportSizeChange` and `ImGuiLayer::begin` to ensure ImGui's IO state is updated before any rendering occurs.
    - [x] Ensure the swapchain/framebuffer is correctly resized and synchronized with ImGui's `DisplaySize`.
- [x] **Task: Implement Robust 0-Size Handling** [commit: 8583632]
    - [x] Ensure `ImGuiLayer::renderDrawData` skips rendering when the window is minimized (framebuffer size is 0, 0) to avoid RHI/Vulkan errors.
- [x] **Task: Conductor - User Manual Verification 'Event Stability'** [commit: 8583632]

## Phase 4: Final Validation and Cleanup
Ensure the fix is robust across all scenarios.

- [x] **Task: Final RHI Validation Audit** [commit: 8583632]
    - [x] Run the application with Vulkan validation layers enabled and perform aggressive resizing.
    - [x] Verify no warnings related to viewport or scissor dimensions are reported.
- [x] **Task: Conductor - User Manual Verification 'Final Validation'** [commit: 8583632]