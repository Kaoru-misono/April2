# Implementation Plan: Fix Editor Resolution and Resize Issues

This plan addresses the low-resolution UI and broken layout/input issues reported during window resizing in the editor.

## Phase 1: Diagnosis and Audit
Identify the root causes of the resolution mismatch and resize instability.

- [x] **Task: Audit `ImGuiLayer` and `Application` Size Handling** [commit: 019b6d5]
    - Examine `engine/graphics/source/graphics/ui/imgui-layer.cpp` and `engine/graphics/source/graphics/ui/application.cpp`.
    - Check how `ImGui::GetIO().DisplaySize` is set.
    - Verify if `glfwGetFramebufferSize` and `glfwGetWindowSize` are used correctly to distinguish between physical and logical coordinates.
- [x] **Task: Diagnostic Logging** [commit: 019b6d5]
    - Add temporary logs to print Window Size, Framebuffer Size, and DPI Scale during initialization and resize.
    - Analyze logs to confirm if the rendering viewport is lagging behind or using the wrong coordinate system.
- [x] **Task: Conductor - User Manual Verification 'Diagnosis'** [commit: d6e1f2b]

## Phase 2: Fix Resolution and DPI Scaling
Ensure the UI renders at the native resolution of the display.

- [x] **Task: Implement High-DPI Support in `ImGuiLayer`** [commit: 019b6d5]
    - Update `ImGuiLayer` to query `glfwGetWindowContentScale` and apply it to `io.FontGlobalScale` or similar if needed.
    - Ensure the rendering viewport (`pCtx->beginRenderPass`) uses the full physical framebuffer size.
- [x] **Task: Standardize Projection Matrix** [commit: 019b6d5]
    - Verify that the ortho projection matrix used for ImGui rendering in `ImGuiLayer::renderDrawData` uses the logical `DisplaySize` while the viewport uses physical pixels.
- [x] **Task: Conductor - User Manual Verification 'Resolution Fix'** [commit: d6e1f2b]

## Phase 3: Robust Resize Handling
Ensure interactions and layout remain consistent during and after resizing.

- [x] **Task: Fix Input Offsets** [commit: 019b6d5]
    - Ensure `ImGui_ImplGlfw_NewFrame` or the manual equivalent is receiving the latest window dimensions every frame.
    - If `io.DisplaySize` is set logical, and rendering is physical, ensure `io.DisplayFramebufferScale` is correctly set to `PhysicalSize / LogicalSize`.
- [x] **Task: Update Integration Test** [commit: 019b6d5]
    - Update `engine/test/test-editor-integration.cpp` to include a check for window resize events and verify that the `Application` continues to render correctly.
- [x] **Task: Conductor - User Manual Verification 'Resize Robustness'** [commit: d6e1f2b]

## Phase 4: Final Validation
Verify the sharpness and interaction accuracy of the UI.

- [x] **Task: Final UI/UX Audit** [commit: d6e1f2b]
    - Confirm text is crisp on high-DPI displays.
    - Confirm all UI panels docked or floating remain interactable after multiple resize cycles.
- [x] **Task: Conductor - User Manual Verification 'Final Validation'** [commit: d6e1f2b]