# Implementation Plan - Refactor ResourceBindFlags

## Phase 1: Core Resource API Refactoring
- [x] Task: Refactor Buffer Class API
    - [x] Update `buffer.hpp` to replace `ResourceBindFlags` with `BufferUsage` in constructors and members.
    - [x] Update `buffer.cpp` implementation to use `BufferUsage`.
    - [x] Update `d3d11-buffer.cpp` and other RHI backend files if necessary (or just the mapping logic).
- [x] Task: Refactor Texture Class API
    - [x] Update `texture.hpp` to replace `ResourceBindFlags` with `TextureUsage` in constructors and members.
    - [x] Update `texture.cpp` implementation to use `TextureUsage`.
- [x] Task: Update RenderDevice Factory Methods
    - [x] Update `RenderDevice::createBuffer` signature and implementation.
    - [x] Update `RenderDevice::createTexture` signature and implementation.
    - [x] Update `RenderDevice` helper functions (e.g., `getFormatBindFlags`).
- [x] Task: Update RHI Tools & Helpers
    - [x] Refactor `rhi-tools` to handle `BufferUsage` and `TextureUsage` separately.
    - [x] Remove `ResourceBindFlags` enum definition (if no longer needed by third-party deps).
- [x] Task: Conductor - User Manual Verification 'Core Resource API Refactoring' (Protocol in workflow.md)

## Phase 2: Engine Subsystem Refactoring
- [x] Task: Refactor GpuProfiler and GpuTimer
    - [x] Update `GpuTimer::create` and usage of buffers.
    - [x] Update `GpuProfiler` internal structures.
- [x] Task: Refactor UI Subsystem
    - [x] Update `ImGuiLayer` vertex/index buffer creation.
    - [x] Update `Font` and other UI elements resource creation.
- [x] Task: Refactor Core/Graphics Utilities
    - [x] Update `ParameterBlock` and other shared graphics utilities.
    - [x] Update any remaining engine source files.
- [x] Task: Conductor - User Manual Verification 'Engine Subsystem Refactoring' (Protocol in workflow.md)

## Phase 3: Test Suite Update and Validation
- [x] Task: Update Unit Tests
    - [x] Update `test-gpu-profiler.cpp`.
    - [x] Update `test-rhi-validation.cpp`.
    - [x] Update `test-nv-template.cpp` and other test files.
- [x] Task: Verify Compilation and Execution
    - [x] Build the entire project.
    - [x] Run all tests to ensure no regressions.
- [x] Task: Conductor - User Manual Verification 'Test Suite Update and Validation' (Protocol in workflow.md)
