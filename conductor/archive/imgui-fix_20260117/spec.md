# Specification: Fix ImGuiLayer Scissor and Window Events

## 1. Overview
This track addresses functional issues in the `ImGuiLayer` implementation, which is a wrapper around `imgui_impl_vulkan.cpp` logic adapted for the April Engine's RHI. Currently, scissor clipping is incorrect, and window events (specifically resizing and DPI scaling) cause visual instability such as flickering or UI disappearance.

## 2. Problem Description
- **Scissor Clipping:** UI elements are not being clipped correctly to their parent containers or the window boundaries, suggesting the RHI `setScissor` calls are receiving incorrect coordinates or scaling.
- **Resize Instability:** Resizing the window leads to flickering or the entire UI disappearing, likely due to improper synchronization between window events and RHI framebuffer/swapchain updates.
- **DPI Scaling:** Changes in DPI (e.g., moving between monitors) are not handled, resulting in incorrect UI sizing or coordinate mismatches.

## 3. Investigation Scope
- **`ImGuiLayer` & `imgui_impl_vulkan.cpp`:** Compare the logic in `engine/graphics/source/graphics/ui/imgui-layer.cpp` with the reference implementation in `imgui_impl_vulkan.cpp`.
- **Coordinate Systems:** Audit the transformation between ImGui's logical coordinates and the RHI's physical pixel coordinates, especially for scissor rectangles.
- **Event Propagation:** Verify how `WindowResizeEvent` and DPI-related scale changes are passed from `GlfwWindow` through the `Application` to `ImGuiLayer`.

## 4. Functional Requirements
- **Accurate Clipping:** Scissor rectangles must be correctly calculated and applied in physical pixels, respecting High-DPI scales.
- **Smooth Resizing:** Resizing the window must not result in visual corruption, flickering, or UI disappearance.
- **DPI Awareness:** The UI must respond correctly to changes in window content scale.

## 5. Acceptance Criteria
- [ ] UI elements are perfectly clipped according to ImGui draw commands.
- [ ] No flickering or UI loss occurs during continuous window resizing.
- [ ] UI scales correctly and maintains input alignment when moved between monitors with different DPI settings.
- [ ] Validation layers remain clean during window transformations.

## 6. Out of Scope
- Implementation of Multi-Viewport (multi-monitor windows) support in the RHI.
- Modification of core RHI command recording logic beyond `ImGuiLayer`.