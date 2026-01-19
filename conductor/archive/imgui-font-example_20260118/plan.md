# Implementation Plan - ImGui Font Texture & Engine Example

## Phase 1: Research & Discovery [checkpoint: 023fd15]
- [x] Task: Analyze `imgui_impl_vulkan` and `nvpro` template
    - [x] Study `imgui_impl_vulkan_CreateFontsTexture` to identify data upload and descriptor binding logic.
    - [x] Deconstruct `nvpro_core2/project_template/main.cpp` to map its lifecycle (Init, Run, Deinit) to April Engine equivalents.
- [x] Task: Audit current April UI and RHI abstractions
    - [x] Identify the specific RHI classes (e.g., `ITexture`, `IDevice`) needed for font creation.
    - [x] Locating the existing `imguiLayer` implementation to determine the best injection point for font creation.
- [x] Task: Conductor - User Manual Verification 'Research & Discovery' (Protocol in workflow.md)

## Phase 2: Font Texture Integration [checkpoint: 1fd46a3]
- [x] Task: TDD - Implement Font Texture Creation in `imguiLayer`
    - [x] Write a failing test in `engine/test/test-ui-editor.cpp` that verifies `imguiLayer` can return a valid handle for the font texture.
    - [x] Implement the font creation logic using April RHI (creating a `Texture`, uploading pixels, and creating a `TextureView`).
    - [x] Update the ImGui descriptor binding to use this new RHI texture.
    - [x] Verify the test passes.
- [x] Task: Refactor and Verify Coverage
    - [x] Ensure font cleanup logic is correctly integrated into `imguiLayer` destruction.
    - [x] Confirm code coverage for the new font creation path.
- [x] Task: Conductor - User Manual Verification 'Font Texture Integration' (Protocol in workflow.md)

## Phase 3: Engine Example Implementation
- [~] Task: Scaffold Example Application
    - [ ] Create `engine/test/test-nv-template.cpp` (or similar) and add it to `CMakeLists.txt`.
    - [ ] Implement the basic windowing and RHI bootstrap using April Engine's core modules.
- [ ] Task: Implement `nvpro` Style UI and Loop
    - [ ] Port the docking and styling configuration from the `nvpro` template.
    - [ ] Implement a basic "Hello World" render loop that includes both an ImGui window and a clear-color background.
- [ ] Task: Final Verification
    - [ ] Execute the built example and manually verify font clarity, docking functionality, and overall UI stability.
- [ ] Task: Conductor - User Manual Verification 'Engine Example Implementation' (Protocol in workflow.md)
