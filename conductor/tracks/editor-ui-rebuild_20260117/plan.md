# Implementation Plan: Game Editor Framework and ImGui Refactor

## Phase 1: ImGui RHI Backend Foundation [checkpoint: 72bf660]
Focus on implementing the modular backend using April's RHI.

- [x] Task: Create `ImGuiBackend` Interface and RHI Implementation (713cfbe)
    - [ ] Define `IImGuiBackend` abstraction in `engine/graphics`.
    - [ ] Implement `SlangRHIImGuiBackend` using April's RHI (Buffers, Textures, Pipelines).
    - [ ] Implement vertex/index buffer management and shader loading for ImGui.
- [x] Task: Refactor `ImGuiLayer` for Modular Usage (6a79883)
    - [ ] Decouple `ImGuiLayer` from direct RHI initialization.
    - [ ] Update `ImGuiLayer` to use the new `ImGuiBackend` for rendering.
- [x] Task: FrameGraph Integration (1568eb4)
    - [ ] Create a dedicated FrameGraph pass for ImGui rendering.
    - [ ] Ensure proper resource transitions for the swapchain image.
- [~] Task: Conductor - User Manual Verification 'ImGui RHI Backend Foundation' (Protocol in workflow.md)

## Phase 2: Editor Framework & Docking System [checkpoint: 961111c]
Establish the professional editor layout.

- [x] Task: Setup ImGui Docking Workspace (9009dbb)
    - [ ] Enable `ImGuiConfigFlags_DockingEnable` and `ImGuiConfigFlags_ViewportsEnable`.
    - [ ] Implement `EditorWorkspace` class to manage the main dockspace.
- [x] Task: Implement Window/DPI Management (c88aeaf)
    - [ ] Hook into GLFW window resize and content scale callbacks.
    - [ ] Implement `nvpro`-style font oversampling and scale adjustment logic.
- [x] Task: Editor Shell Implementation (77e0ac6)
    - [ ] Create the main menu bar (File, Edit, View, Help).
    - [ ] Implement the base `EditorPanel` class.
- [x] Task: Conductor - User Manual Verification 'Editor Framework & Docking System' (Protocol in workflow.md) (961111c)

## Phase 3: Core Editor Panels [checkpoint: f24a33e]
Implement the visual framework for the five required panels.

- [x] Task: Viewport Panel (6f2f431)
    - [x] Implement offscreen rendering to a texture via RHI.
    - [x] Display the offscreen texture within an ImGui window with correct aspect ratio.
- [x] Task: Scene Hierarchy & Inspector Panels (67f2450)
    - [x] Create basic tree-view for Hierarchy.
    - [x] Implement a generic property reflecting system for the Inspector.
- [x] Task: Content Browser & Console Panels (67f2450)
    - [x] Implement a file-system based asset viewer.
    - [x] Create a thread-safe logging sink for the Editor Console.
- [x] Task: Conductor - User Manual Verification 'Core Editor Panels' (Protocol in workflow.md) (f24a33e)

## Phase 4: Polish & Validation
Final adjustments to ensure stability and visual quality.

- [~] Task: Font Sharpness & HiDPI Validation
    - [ ] Verify font rendering on different display scales.
    - [ ] Fix any blurring issues during window moves/resizes.
- [ ] Task: Final Cleanup & Regression Testing
    - [ ] Remove any remaining references to `nvpro_core2` libraries.
    - [ ] Ensure all existing engine tests pass with the new UI system.
- [ ] Task: Conductor - User Manual Verification 'Polish & Validation' (Protocol in workflow.md)
