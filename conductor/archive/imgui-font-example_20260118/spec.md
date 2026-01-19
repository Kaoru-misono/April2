# Specification - ImGui Font Texture & Engine Example

## Overview
This track focuses on completing the Dear ImGui integration by implementing the font texture creation logic using the April engine's custom RHI. Additionally, a new example application will be created that mirrors the structural and visual setup of the `nvpro_core2` project template, but built entirely on the April engine abstractions.

## Functional Requirements
- **Font Texture Integration:**
    - Analyze `imgui_impl_vulkan` to understand how the font atlas is generated and uploaded.
    - Implement a corresponding `create_font_texture` (or similar) method within the `imguiLayer` (or appropriate UI component) using the April RHI.
    - Strictly use April RHI high-level abstractions (`Texture`, `TextureView`) for texture management.
    - Ensure the ImGui font atlas is correctly bound to the descriptor sets/pipeline used by the April UI renderer.
- **Engine Example (`nvpro` Style):**
    - Create a new example/test file mirroring `nvpro_core2/project_template/main.cpp`.
    - Use April's core and graphics modules for window creation, RHI initialization, and the main loop.
    - Implement the standard ImGui setup (docking, style) as seen in the `nvpro` template.
    - Ensure the example demonstrates both basic engine rendering and the functional ImGui UI.

## Non-Functional Requirements
- **Code Consistency:** Follow the existing C++23 patterns and naming conventions found in `engine/graphics` and `engine/ui`.
- **Encapsulation:** Maintain strict encapsulation of the RHI; no raw Vulkan calls should be introduced into the UI layer.

## Acceptance Criteria
- [ ] The font texture is successfully created and rendered in ImGui (no more default "no texture" blocks).
- [ ] A new example application is added to the project (e.g., in `engine/test` or a new examples directory).
- [ ] The example runs correctly, showing an ImGui interface with working fonts and a functional docking workspace.
- [ ] The example does not depend on `nvpro_core2` headers or libraries.

## Out of Scope
- Full parity with all `nvpro_core2` features (only the basic template structure is required).
- Implementing complex custom ImGui widgets (unless they are part of the base template).
