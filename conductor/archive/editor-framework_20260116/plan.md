# Implementation Plan: Editor Framework & Widget Integration

This plan implements a modular editor framework for the April Engine, adopting the architectural patterns of `nvapp` and the widget library of `nvgui`.

## Phase 1: Foundation & Modular Framework
Establish the core interfaces and the `Application` registry.

- [x] **Task: Define `IAppElement` Interface** 89b28ed
    - Create `engine/graphics/source/graphics/ui/app-element.hpp` defining the interface (onAttach, onDetach, onUIRender, onRender, etc.) matching the `nvapp::IAppElement` pattern.
- [x] **Task: Implement `Application` Manager** 4701120
    - [ ] Write tests in `engine/test/test-editor-framework.cpp` for element registration and lifecycle calls.
    - [ ] Implement `Application` to manage a collection of `IAppElement` pointers.
    - [ ] Integrate styling and font initialization into `Application::init()`.
- [x] **Task: Engine Integration** 6da9bca
    - Hook `Application` into the main `Engine` class's update and render paths (via sample integration).
- [x] **Task: Conductor - User Manual Verification 'Foundation & Modular Framework' (Protocol in workflow.md)** 78fb174
[checkpoint: 78fb174]

## Phase 2: Workspace & Docking
Implement the docking infrastructure and main workspace layout.

- [x] **Task: Implement Docking Logic** 23ea39d
    - [ ] Port `setupImguiDock` logic from `nvapp::Application` to `Application`.
    - [ ] Implement the `dockSetup` callback mechanism to allow flexible layout definitions.
- [x] **Task: Create Main Workspace Element** cc52aa3
    - Create a "MainWorkspace" element that handles the `DockSpaceOverViewport` and main menu bar calls.
- [x] **Task: Conductor - User Manual Verification 'Workspace & Docking' (Protocol in workflow.md)** 5a0d0f6
[checkpoint: 5a0d0f6]

## Phase 3: Core Elements (Viewport & Camera)
Implement the 3D scene viewport and navigation.

- [x] **Task: Implement `SceneViewportElement`** 373fe3e
    - [ ] Create a class implementing `IAppElement`.
    - [ ] Handle rendering the engine's offscreen texture into an ImGui window named "Viewport".
    - [ ] Implement resize handling to update the engine's render target size.
- [x] **Task: Implement `CameraElement`** a837110
    - [ ] Adapt `nvgui/camera.cpp` logic into a reusable element.
    - [ ] Link camera input handling to the "Viewport" window's focus/hover state.
- [x] **Task: Conductor - User Manual Verification 'Core Elements' (Protocol in workflow.md)** 4b3a4f9
[checkpoint: 4b3a4f9]

## Phase 4: Property & Rendering Widgets
Integrate the specialized UI components from `nvgui`.

- [x] **Task: Implement `PropertyInspectorElement`** 08ea085
    - [ ] Create an element that uses `nvgui/property_editor.hpp` to display object properties.
    - [ ] Implement basic transform editing (Position, Rotation, Scale).
- [x] **Task: Implement `RenderingControlElement`** d82f28c
    - [ ] Create an element to host the "Rendering" panel.
    - [ ] Integrate `nvgui::tonemapperWidget`.
    - [ ] Integrate `nvgui::skySimpleParametersUI`.
- [x] **Task: Conductor - User Manual Verification 'Property & Rendering Widgets' (Protocol in workflow.md)** ca5444e
[checkpoint: ca5444e]

## Phase 5: Polish & Validation
Final adjustments to match the professional look and feel of the reference samples.

- [x] **Task: UI/UX Polish** e2f58a4
    - Refine docking defaults and window constraints.
    - Ensure font scaling and DPI awareness (referencing `nvapp` logic).
- [~] **Task: Final Integration Test**
    - Verify that all elements interact correctly and state is preserved where applicable.
- [ ] **Task: Conductor - User Manual Verification 'Polish & Validation' (Protocol in workflow.md)**