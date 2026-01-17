# Plan - ImGui Refactor (Modular Editor Framework)

## Phase 1: Modular Foundation
- [x] Task: Define the `EditorElement` interface.
    - [x] Create `engine/graphics/source/graphics/ui/editor-element.hpp`.
    - [x] Define virtual methods: `onAttach`, `onDetach`, `onUIRender`, `onRender`, `onResize`.
- [~] Task: Refactor `ImGuiLayer` to support elements.
    - [ ] Add `std::vector<std::shared_ptr<EditorElement>>` to `ImGuiLayer`.
    - [ ] Implement `addElement` and `removeElement` methods.
    - [ ] Update `begin()` and `end()` to iterate through elements and call their hooks.
- [x] Task: Conductor - User Manual Verification 'Phase 1: Modular Foundation' (Protocol in workflow.md)

## Phase 2: Docking and Offscreen Viewport
- [x] Task: Implement Automated Docking.
    - [x] Add `setupDockspace` to `ImGuiLayer` using `ImGui::DockSpaceOverViewport`.
    - [x] Implement basic layout builder (splitting nodes for "Viewport" and "Settings").
- [x] Task: Create `ViewportElement`.
    - [x] Implement offscreen framebuffer management within `ViewportElement`.
    - [x] Use ImGui `Image` to display the offscreen texture in the "Viewport" window.
    - [x] Ensure `onResize` correctly updates the offscreen framebuffer size.
- [x] Task: Conductor - User Manual Verification 'Phase 2: Docking and Offscreen Viewport' (Protocol in workflow.md)

## Phase 3: Robust DPI and Mode Switching
- [x] Task: Implement DPI Scaling Sync.
    - [x] Refine `ImGuiLayer::begin` to use `glfwGetWindowContentScale` correctly.
    - [x] Adjust `io.FontGlobalScale` and display size dynamically.
- [x] Task: Implement Hybrid Rendering Modes.
    - [x] Add a toggle between "Editor Mode" and "Game Mode".
    - [x] In "Game Mode", bypass the dockspace and offscreen rendering, rendering directly to swapchain.
- [x] Task: Conductor - User Manual Verification 'Phase 3: Robust DPI and Mode Switching' (Protocol in workflow.md)

## Phase 4: Integration and Sample Elements
- [x] Task: Port existing functionality to Elements.
    - [x] Create a `DefaultMenuBarElement`.
    - [x] Create a `SceneRenderingElement` (porting current non-UI rendering logic).
- [x] Task: Final System Verification.
    - [x] Run the editor and verify docking persistence (via `.ini`).
    - [x] Stress test window resizing and DPI changes.
- [x] Task: Conductor - User Manual Verification 'Phase 4: Integration and Sample Elements' (Protocol in workflow.md)