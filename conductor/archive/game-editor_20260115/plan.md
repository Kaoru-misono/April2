# Plan: Game Editor (Initial Foundation)

## Phase 1: Editor Infrastructure [checkpoint: a744fb7]
- [x] Task: Implement EditorSystem Scaffolding (TDD) 0c7cd15
    - [x] Subtask: Write unit tests for `EditorSystem` lifecycle (startup/shutdown).
    - [x] Subtask: Create `EditorSystem` class and integrate into the engine modular system.
- [x] Task: ImGui Docking and Theme Initialization 0c7cd15
    - [x] Subtask: Enable ImGui Docking flags and initialize docking workspace.
    - [x] Subtask: Implement the "Professional Dark" theme customization.
- [x] Task: Conductor - User Manual Verification 'Editor Infrastructure' (Protocol in workflow.md) a744fb7

## Phase 2: Offscreen Viewport Rendering [checkpoint: fee72d5]
- [x] Task: Setup Viewport RHI Resources (TDD) 8de4d4d
    - [x] Write unit tests verifying offscreen texture and render target creation.
    - [x] Implement offscreen RHI texture and RenderTargetView for the viewport.
- [x] Task: Redirect Scene Rendering 8de4d4d
    - [x] Implement logic to render the main game scene into the offscreen buffer instead of the swapchain.
- [x] Task: ImGui Viewport Window 8de4d4d
    - [x] Create a dedicated ImGui window that displays the offscreen texture.
    - [x] Handle window resizing to update the offscreen texture dimensions.
- [x] Task: Conductor - User Manual Verification 'Offscreen Viewport Rendering' (Protocol in workflow.md) fee72d5

## Phase 3: Viewport Interaction & Overlays [checkpoint: 5359dcb]
- [x] Task: Implement Editor Camera System (TDD) 7f1690e
    - [x] Write unit tests for camera transformation and input mapping logic.
    - [x] Implement basic fly-through camera movement.
    - [x] Add logic to restrict camera inputs to when the ImGui Viewport window is focused.
- [x] Task: Implement Viewport Overlay UI 7f1690e
    - [x] Add 2D ImGui overlay panels/buttons on top of the 3D viewport.
    - [x] Bind overlay controls to camera parameters (e.g., FOV, speed).
- [x] Task: Conductor - User Manual Verification 'Viewport Interaction' (Protocol in workflow.md) 5359dcb

## Phase 4: Integration and Final Polish [checkpoint: 1f9d70d]
- [x] Task: Modular Toggle Integration 7809fd5
    - [x] Ensure the editor can be cleanly enabled or disabled via engine configuration.
- [x] Task: Final Cleanup and Documentation 7809fd5
    - [x] Refactor and document public `EditorSystem` API.
    - [x] Verify compliance with project code styleguides.
- [x] Task: Conductor - User Manual Verification 'Integration and Polish' (Protocol in workflow.md) 1f9d70d
