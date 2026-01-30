# Implementation Plan - RenderPassEncoder Blit

## Phase 1: Foundation & Shader
- [x] Task: Create `blit.slang` shader [12267d5]
    - [ ] Create `engine/graphics/shader/blit.slang` with vertex and fragment entry points.
    - [ ] Implement a full-screen triangle vertex shader (generating UVs from vertex index).
    - [ ] Implement a fragment shader that samples a texture using UVs and a sampler.
    - [ ] Define shader parameters (Texture2D, SamplerState).
- [ ] Task: Conductor - User Manual Verification 'Phase 1' (Protocol in workflow.md)

## Phase 2: RenderPassEncoder Implementation
- [ ] Task: Setup Blit Internal State
    - [ ] Add `BlitPipeline` struct/class to `RenderPassEncoder` (or `CommandContext` if shared) to hold the pipeline state.
    - [ ] Initialize the blit pipeline on demand (lazy initialization).
- [ ] Task: Implement `RenderPassEncoder::blit`
    - [ ] Implement the `blit` method in `engine/graphics/source/graphics/rhi/command-context.cpp`.
    - [ ] Configure the `Viewport` and `Scissor` based on `dstRect`.
    - [ ] Bind the `blit` pipeline.
    - [ ] Bind the source texture (`src`) and sampler (based on `filter`).
    - [ ] Issue a draw call (3 vertices) for the full-screen triangle.
- [ ] Task: Conductor - User Manual Verification 'Phase 2' (Protocol in workflow.md)

## Phase 3: Testing & Verification
- [ ] Task: Create Unit Test
    - [ ] Create `engine/test/test-blit.cpp`.
    - [ ] Setup a test case with a known source texture (e.g., a simple checkerboard or solid color pattern).
    - [ ] Create a destination render target.
    - [ ] Execute `RenderPassEncoder::blit` with various parameters (different rects, scaling).
    - [ ] Read back the destination texture to CPU.
    - [ ] Verify pixel values against expected results.
- [ ] Task: Conductor - User Manual Verification 'Phase 3' (Protocol in workflow.md)
