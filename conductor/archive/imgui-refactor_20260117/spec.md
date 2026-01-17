# Specification - ImGui Refactor (Modular Editor Framework)

## Overview
Refactor the existing `ImGuiLayer` into a modular editor framework inspired by the `nvpro_core2` architecture. This will enable a decoupled system where various editor components (elements) can be added or removed independently, supporting advanced docking, offscreen viewport rendering, and robust DPI scaling.

## Functional Requirements
- **Modular Element System:** Implement an `EditorElement` interface with hooks for `onAttach`, `onDetach`, `onUIRender`, `onRender`, and `onResize`.
- **Advanced Docking:** Implement automated dockspace setup with a central "Viewport" and peripheral utility windows (e.g., "Settings", "Log").
- **Hybrid Rendering Modes:**
    - **Editor Mode:** Render the scene to an offscreen texture and display it within an ImGui window.
    - **Game Mode:** Render directly to the swapchain with ImGui as a minimalist overlay.
- **Robust DPI Scaling:** Synchronize ImGui's font and display scaling with the platform's content scale factors.
- **RHI Integration:** Elements must receive a `CommandContext*` during `onRender` to perform RHI-based operations.

## Non-Functional Requirements
- **Maintainability:** Decouple engine-level ImGui backend management from application-level UI logic.
- **Extensibility:** Allow users to easily add custom editor elements without modifying the core `ImGuiLayer`.

## Acceptance Criteria
- A functional dockable workspace is created upon startup.
- The "Viewport" window correctly displays an offscreen-rendered scene with correct aspect ratio handling.
- Elements can perform both ImGui UI calls and direct RHI commands.
- High-DPI displays are handled correctly without blurry text or misaligned input.

## Out of Scope
- Implementing specific game logic inside the editor elements.
- Rewriting the underlying `slang-rhi` or ImGui backends.