# Implementation Plan: Fix Editor Viewport and Docking Issues

This plan addresses the layout conflict between Game/Editor modes and ensures correct viewport sizing in the Editor.

## Phase 1: Separate ImGui Configurations [checkpoint: 7f49b6a]
Prevent layout corruption by assigning distinct `.ini` files for Game and Editor.

- [x] Task: Update `EngineConfig` and `ImGuiLayer` to support custom `.ini` paths.
    - [x] Modify `EngineConfig` struct in `engine.hpp` to add `std::string imguiIniFilename`.
    - [x] Update `ImGuiLayer::init` to use this config value.
- [x] Task: Update Entry Points.
    - [x] Modify `entry/editor/main.cpp` to set `config.imguiIniFilename = "imgui_editor.ini"`.
    - [x] Modify `entry/game/main.cpp` to set `config.imguiIniFilename = "imgui_game.ini"`.
- [x] Task: Verify Configuration Separation.
    - [x] Run Editor and Game, check if separate `.ini` files are created.
    - [x] Conductor - User Manual Verification (Protocol in workflow.md).

## Phase 2: Fix Editor Viewport Sizing [checkpoint: 7f49b6a]
Ensure the Editor controls the 3D scene resolution, overriding the default Engine window-resize behavior.

- [x] Task: Investigate and Fix Viewport Logic.
    - [x] Analyze `EditorViewport` (or equivalent class in `engine/editor`) to see how it sets the scene size.
    - [x] Modify `Engine` or `EditorApp` to ensure `SceneRenderer::setViewportSize` is driven by the ImGui panel size in Editor mode.
    - [x] *Potential Fix:* In `EditorApp::onUI` (or where the viewport panel is drawn), call `engine.setSceneViewportSize(panelWidth, panelHeight)` and `engine.ensureOffscreenTarget(...)`.
    - [x] Disable/Ignore the automatic `SceneRenderer` resize in `Engine::renderFrame` *if* the viewport size is being managed externally (or ensure the external management overrides it efficiently).
- [x] Task: Verify Viewport Resolution.
    - [x] Launch Editor, resize window, resize Viewport panel. Verify aspect ratio and resolution.
    - [x] Conductor - User Manual Verification (Protocol in workflow.md).
