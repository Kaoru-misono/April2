# Plan - ImGui Scissor Fix (RHI Integration)

## Phase 1: Analysis and Comparison
- [x] Task: Audit existing ImGui RHI implementation against `imgui_impl_vulkan.cpp`.
    - [x] Locate the current ImGui rendering loop in the codebase.
    - [x] Compare the calculation of `clip_rect` and scissor parameters with the reference implementation.
    - [x] Identify where `DisplayPos` and `FramebufferScale` are applied (or missing).
- [ ] Task: Conductor - User Manual Verification 'Phase 1: Analysis and Comparison' (Protocol in workflow.md)

## Phase 2: Implementation of Fixed Scissor Logic
- [x] Task: Refactor coordinate transformation logic.
    - [x] Implement robust calculation for scissor origin and extent using `DisplayPos` and `FramebufferScale`.
    - [x] Ensure proper clamping of the scissor rectangle to the swapchain/render target extents.
- [x] Task: Update RHI setScissorRect calls.
    - [x] Verify that the calculated rect is passed correctly to the April RHI `setScissorRect` equivalent.
    - [x] Add debug logging to output calculated scissor rect values for verification.
- [x] Task: Conductor - User Manual Verification 'Phase 2: Implementation of Fixed Scissor Logic' (Protocol in workflow.md)

## Phase 3: Verification and Testing
- [x] Task: Manual Editor Verification.
    - [x] Run the application/editor and perform stress tests on window clipping (dragging windows partially off-screen).
    - [x] Verify behavior with high-DPI settings if applicable.
- [x] Task: Log Analysis.
    - [x] Review the debug logs to ensure scissor rect values align with ImGui's expected bounds.
- [x] Task: Conductor - User Manual Verification 'Phase 3: Verification and Testing' (Protocol in workflow.md)