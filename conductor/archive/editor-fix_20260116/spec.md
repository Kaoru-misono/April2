# Specification: Fix Editor Resolution and Resize Issues

## 1. Overview
This track aims to investigate and resolve rendering quality and interaction issues within the Editor's ImGui integration. The user reports that the editor window displays content but with very low resolution, and resizing the window breaks the ImGui content layout and interaction (input offsets). The investigation will primarily focus on `ImGuiLayer` and its handling of window resize events and framebuffer sizing.

## 2. Problem Description
- **Low Resolution:** The editor UI appears blurry or pixelated, suggesting a mismatch between the rendering resolution and the window's display size, or a lack of DPI scaling support.
- **Resize Instability:** Resizing the application window causes ImGui content to break. Specifically, mouse inputs become offset from the visual UI elements.
- **Context:** The user recently modified viewport-related code in `ImGuiLayer`, which seemingly fixed a "black screen" issue but introduced these new problems.

## 3. Investigation Scope
- **`ImGuiLayer` Class:** Analyze how `ImGui` is initialized, how frame buffers are sized, and how render data is generated.
- **Window Resize Handling:** Verify how resize events are propagated to `ImGui`. Check `ImGui_ImplGlfw_NewFrame` and `ImGui::GetIO().DisplaySize`.
- **DPI/Scale Handling:** Check if `glfwGetWindowContentScale` or similar framebuffer size queries are correctly used to set up the rendering viewport and ImGui IO.

## 4. Functional Requirements
- **Correct Resolution:** The Editor UI must render at the native resolution of the window/monitor (1:1 pixel mapping) to ensure sharpness.
- **Robust Resizing:** Resizing the window must dynamically update the rendering context and ImGui display size without breaking layout or input handling.
- **Input Accuracy:** Mouse clicks must accurately register on UI elements after resizing.

## 5. Acceptance Criteria
- [ ] The Editor UI appears sharp and clear (not pixelated) on the user's display.
- [ ] Resizing the window updates the UI layout smoothly without visual corruption.
- [ ] Mouse interactions (clicks, hovers) remain accurate and aligned with visual elements after resizing the window.
- [ ] `ImGuiLayer` correctly handles framebuffer size vs. window size (DPI awareness).

## 6. Out of Scope
- Fixing 3D scene rendering issues (as the user stated the scene is empty).
- Implementing new UI features.
