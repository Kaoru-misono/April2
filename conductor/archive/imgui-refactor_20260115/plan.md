# Plan: ImGui Layer RHI Refactor and Validation

## Phase 1: Analysis & Infrastructure [checkpoint: 3d4fb04]
- [x] Task: Audit existing ImGui implementation 33987ac
    - [x] Identify all direct API calls (D3D12/Vulkan) in `imgui-layer.cpp/hpp`.
    - [x] Map existing ImGui state (shaders, buffers, textures) to RHI equivalents.
- [x] Task: Set up Dedicated Test Environment 33987ac
    - [x] Create `engine/test/test-imgui.cpp` as the entry point for the new test executable.
    - [x] Update `engine/test/CMakeLists.txt` to build `test-imgui` linking `April_graphics`.
- [x] Task: Conductor - User Manual Verification 'Analysis & Infrastructure' (Protocol in workflow.md) 6a6e9a2

## Phase 2: RHI Resource Foundation [checkpoint: 8c31854]
- [x] Task: Implement ImGui RHI Unit Tests (Red Phase) d68567e
    - [x] Write tests in `test-imgui.cpp` for font texture creation and buffer management.
    - [x] Write tests for ImGui-specific PSO (Pipeline State Object) creation through RHI.
- [x] Task: Refactor Resource Creation (Green Phase) d68567e
    - [x] Update `ImGuiLayer` to use `april::graphics` for font texture and sampler handles.
    - [x] Implement vertex and index buffer management using RHI `IBuffer`.
- [x] Task: Conductor - User Manual Verification 'RHI Resource Foundation' (Protocol in workflow.md) 8c31854

## Phase 3: Backend Rendering Refactor [checkpoint: bbdaa6a]
- [x] Task: Implement Draw Command Validation Tests (Red Phase) d41f502
    - [x] Write mock tests to verify that ImGui draw lists are correctly parsed.
    - [x] Write tests for clipping rectangle and viewport state application.
- [x] Task: Refactor Rendering Logic (Green Phase) d41f502
    - [x] Replace direct command list calls with `ICommandQueue` and `ICommandEncoder`.
    - [x] Implement the main render loop in `ImGuiLayer` using the RHI abstraction.
- [x] Task: Conductor - User Manual Verification 'Backend Rendering Refactor' (Protocol in workflow.md) 5fd5020

## Phase 4: Integration & Visual Validation [checkpoint: daaea73]
- [x] Task: Implement End-to-End Visual Tests 340e6b0
    - [x] Create a "Hello World" ImGui window test in `test-imgui`.
    - [x] Verify rendering across D3D12 and Vulkan backends.
- [x] Task: Final Cleanup & Documentation 340e6b0
    - [x] Remove all legacy API include guards and code.
    - [x] Ensure compliance with `code_styleguides/general.md`.
- [x] Task: Conductor - User Manual Verification 'Integration & Visual Validation' (Protocol in workflow.md) daaea73
