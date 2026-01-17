# Specification: Game Editor Framework and ImGui Refactor

## 1. Overview
Refactor April's UI system to create a modular, high-performance Game Editor framework. This involves moving away from the current `ImGuiLayer` implementation to a more robust architecture that leverages April's internal RHI and integrates with the FrameGraph, providing a professional docking workspace with sharp, DPI-aware rendering.

## 2. Functional Requirements
- **Modular Architecture:** 
    - Implement a standalone `ImGuiBackend` that wraps April RHI calls.
    - Create an `ImGuiLayer` for state management and UI logic.
    - Integrate the system as a first-class engine component interacting with the FrameGraph/TaskGraph.
- **Game Editor Layout:** Implement a docking-enabled workspace featuring:
    - **Viewport:** Offscreen rendering support for the main game scene.
    - **Inspector:** Property editor for engine objects.
    - **Scene Hierarchy:** Tree view of the scene structure.
    - **Content Browser:** Asset management interface.
    - **Console/Logger:** Diagnostic output panel.
- **Resize & DPI Handling:**
    - Robust handling of window resize events.
    - Sharp font rendering using 3x oversampling and dynamic `FontGlobalScale` adjustments based on monitor DPI.
- **Dependency Management:**
    - Reference `nvpro_core2` patterns but implement using April's `Core`, `Math`, and `RHI` libraries.
    - Eliminate any direct library dependency on `nvpro_core2`.

## 3. Non-Functional Requirements
- **C++23 Compliance:** Use modern C++23 features (modules, concepts) where appropriate.
- **Performance:** Minimal overhead for UI rendering and synchronization.
- **Visual Fidelity:** High-quality, non-blurry text and UI elements on all display types.

## 4. Acceptance Criteria
- [ ] Engine boots into a professional docking layout.
- [ ] All 5 core panels (Viewport, Inspector, Hierarchy, Content, Console) are visible and dockable.
- [ ] Resizing the main window or individual dock nodes does not cause rendering artifacts or blurring.
- [ ] UI text remains sharp when moving between monitors with different DPI scales.
- [ ] Codebase is free of `nvpro_core2` library calls, using April's internal abstractions instead.

## 5. Out of Scope
- Finalizing the deep logic of every panel (e.g., actual asset importing/drag-and-drop). This track focuses on the **framework, rendering, and layout**.
