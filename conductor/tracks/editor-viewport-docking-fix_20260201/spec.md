# Specification: Fix Editor Viewport and Docking Issues

## 1. Overview
Users report that switching between `April_game` and `April_editor` causes the Editor's viewport resolution to incorrectly match the OS window size instead of the ImGui Viewport panel size. Additionally, docking components appear broken. This track aims to decouple the Game and Editor ImGui configurations and ensure the Editor correctly manages the 3D scene viewport resolution.

## 2. Problem Analysis
- **Viewport Resolution:** The `Engine` class (recently modified) resizes the `Offscreen` target and `SceneRenderer` viewport to the *OS Window* size on resize events. While this is correct for the `Game` runtime, the `Editor` displays the scene in a smaller ImGui panel. The Editor must override or manage the `SceneRenderer` viewport size independently of the OS window resize event.
- **Docking/Layout:** `April_game` and `April_editor` likely share the same `imgui.ini` file in the project root. The Game's ImGui context (likely non-docking or simple) overwrites the complex Editor docking layout, corrupting it on the next Editor launch.

## 3. Functional Requirements
- **Independent Layouts:** `April_editor` and `April_game` MUST use separate ImGui configuration files (e.g., `imgui_editor.ini` and `imgui_game.ini`) to prevent layout conflicts.
- **Correct Viewport Resize:** The 3D Scene in the Editor MUST render at the resolution and aspect ratio of the "Viewport" ImGui panel, not the OS window.
- **Real-Time Resize:** The previously implemented real-time resize fix MUST still work (UI updates while dragging), but the 3D scene projection must be correct.

## 4. Technical Implementation
- **ImGui Config Separation:**
    - Update `EngineConfig` to allow specifying a custom `ini` filename.
    - Update `EditorApp` (entry point) to set `imgui.iniFilename = "imgui_editor.ini"`.
    - Update `Game` entry point to set `imgui.iniFilename = "imgui_game.ini"`.
- **Viewport Logic:**
    - Investigate `EditorApp` or `EditorViewport` class.
    - Ensure that during the ImGui render loop, the `SceneRenderer`'s viewport size is updated *only* when the ImGui Viewport panel size changes.
    - Disable the `Engine`'s default behavior of automatically updating the `SceneRenderer` viewport on window resize *if* running in Editor mode (or allow the Editor to override it every frame).

## 5. Acceptance Criteria
- [ ] Run `April_game`, close it.
- [ ] Run `April_editor`.
- [ ] Verify that the Editor docking layout is preserved (or resets to a clean default if `imgui_editor.ini` is new).
- [ ] Verify that the 3D scene inside the "Viewport" panel has the correct aspect ratio and doesn't look stretched or pixelated relative to the panel size.
- [ ] Resize the OS window; verify the Viewport panel resizes correctly and the 3D scene adjusts to the *panel* size.

## 6. Out of Scope
- Redesigning the entire UI framework.
