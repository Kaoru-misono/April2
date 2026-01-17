# Plan: Refactor RHI Command Context and Pipelines

## Phase 1: Pipeline Classes Implementation [checkpoint: 0dbc67b]
- [x] Task: Create Pipeline Classes (Refactor Existing)
    - [x] Subtask: Create `GraphicsPipeline` class (referencing `GraphicsStateObject` and existing code from `d8336ea`).
    - [x] Subtask: Create `ComputePipeline` class (referencing `ComputeStateObject`).
    - [x] Subtask: Create `RayTracingPipeline` class (referencing `RtStateObject`).
    - [x] Subtask: Ensure these classes expose necessary descriptors/handles for the RHI backend.
- [x] Task: Refactor Command Context (Continue Implementation)
    - [x] Subtask: Continue implementation of `CommandContext` (building on your existing `command-context.hpp` and `.cpp`).
    - [x] Subtask: Refine `CommandContext` to accept the new Pipeline types.
    - [x] Subtask: Implement strict capability checks (e.g., assert or throw if binding a GraphicsPipeline on a Compute-only context).
- [x] Task: Conductor - User Manual Verification 'Pipeline Classes Implementation' (Protocol in workflow.md) 0dbc67b

## Phase 2: Command Context Unification [checkpoint: 1825af1]
- [x] Task: Implement Copy Commands
    - [x] Subtask: Migrate logic from `CopyContext` to `CommandContext`.
    - [x] Subtask: Add validation to ensure copy commands work on Copy, Compute, and Graphics queues.
- [x] Task: Implement Compute Commands
    - [x] Subtask: Migrate logic from `ComputeContext` to `CommandContext`.
    - [x] Subtask: Add validation to ensure compute commands work only on Compute and Graphics queues.
- [x] Task: Implement Graphics Commands
    - [x] Subtask: Migrate logic from `RenderContext` to `CommandContext`.
    - [x] Subtask: Add validation to ensure graphics commands work only on Graphics queues.
- [x] Task: Implement Ray Tracing Commands
    - [x] Subtask: Migrate logic from specialized ray tracing paths to `CommandContext`.
- [x] Task: Conductor - User Manual Verification 'Command Context Unification' (Protocol in workflow.md) 1825af1

## Phase 3: Transition & Cleanup
- [x] Task: Update Application Code
    - [x] Subtask: Search for all usages of `GraphicsStateObject` and replace with `GraphicsPipeline`.
    - [x] Subtask: Search for all usages of `RenderContext`, `ComputeContext`, `CopyContext` and replace with `CommandContext`.
- [x] Task: Verify Tests
    - [x] Subtask: Update `test-triangle.cpp` to use the new `CommandContext` and `GraphicsPipeline`.
    - [x] Subtask: Update `test-object.cpp` to use the new system.
    - [x] Subtask: Run tests and fix any compilation or runtime errors.
- [x] Task: Delete Legacy Files & Finalize Naming
    - [x] Subtask: Delete `CopyContext`, `RenderContext`, `ComputeContext`.
    - [x] Subtask: Delete `GraphicsStateObject`, `ComputeStateObject`, `RtStateObject`.
    - [x] Subtask: Verify all new files use correct kebab-case naming (e.g., `graphics-pipeline.hpp`, `command-context.hpp`).
- [x] Task: Conductor - User Manual Verification 'Transition & Cleanup' (Protocol in workflow.md) ac1b0c0

## Phase 4: Visual Debugging (User)
- [x] Task: Debug Visual Issue (Triangle not appearing) 81e4243
    - [x] Subtask: Refactor: Remove FrameBuffer and use ColorTarget/DepthStencilTarget directly.
    - [x] Subtask: Investigate `CommandContext` state management and pass execution.
    - [x] Subtask: Verify `prepareDescriptorSets` and resource transitions.
    - [x] Subtask: Confirm triangle is visible in `test-triangle.cpp`.
- [x] Task: Create isolated slang-rhi triangle test 1693f03
    - [x] Subtask: Create `test-slang-rhi.cpp` using only `slang-rhi` and `glfw`.
    - [x] Subtask: Implement direct initialization and rendering loop.
    - [x] Subtask: Verify triangle visibility in isolation.
