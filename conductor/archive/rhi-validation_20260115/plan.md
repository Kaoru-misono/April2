# Plan: Comprehensive RHI Validation Suite

## Phase 1: Foundation (Data & Resources) [checkpoint: c16bb98]
- [x] Task: Validate Buffer & Texture Data Integrity c16bb98
    - [x] Subtask: Write tests for uploading data to Buffers/Textures and reading it back.
    - [x] Subtask: Implement RHI-level validation for data transfer across D3D12 and Vulkan.
- [x] Task: Validate Sampler & Filter Mode Operations c16bb98
    - [x] Subtask: Write tests for creating samplers with various filter/addressing modes.
    - [x] Subtask: Implement validation for sampler state application in shaders.
- [x] Task: Conductor - User Manual Verification 'Foundation' (Protocol in workflow.md)

## Phase 2: Execution (Compute & Graphics) [checkpoint: 6fcd588]
- [x] Task: Validate Compute Dispatch & UAV Operations 6fcd588
    - [x] Subtask: Write tests for dispatching compute shaders and writing to UAVs.
    - [x] Subtask: Verify UAV data consistency after execution.
- [x] Task: Validate Basic Rasterization Pipeline 6fcd588
    - [x] Subtask: Write tests for drawing primitives using a simple Graphics PSO.
    - [x] Subtask: Implement validation for vertex fetching and pixel output.
- [x] Task: Conductor - User Manual Verification 'Execution' (Protocol in workflow.md)

## Phase 3: Advanced Features (Memory & Ray Tracing) [checkpoint: 0a82aa6]
- [x] Task: Validate Ray Tracing Acceleration & Execution 6fcd588
    - [x] Subtask: Write tests for building Bottom-Level and Top-Level Acceleration Structures.
    - [x] Subtask: Implement a minimal TraceRay test to verify hit groups.
- [x] Task: Validate Manual Heaps & Resource Aliasing 0a82aa6
    - [x] Subtask: Write tests for manual memory allocation from RHI Heaps.
    - [x] Subtask: Implement validation for aliasing multiple textures/buffers on the same memory.
- [x] Task: Conductor - User Manual Verification 'Advanced Features' (Protocol in workflow.md)

## Phase 4: Integration & Optimization [checkpoint: 7fa3d4a]
- [x] Task: Validate Swapchain & Window Presentation 7fa3d4a
    - [x] Subtask: Write tests for creating a swapchain linked to a GLFW window.
    - [x] Subtask: Verify frame presentation and front/back buffer flipping.
- [x] Task: Validate PSO Caching & Persistence 7fa3d4a
    - [x] Subtask: Write tests for serializing and deserializing PSO caches.
    - [x] Subtask: Verify that reloaded PSOs function identically to freshly compiled ones.
- [x] Task: Conductor - User Manual Verification 'Integration & Optimization' (Protocol in workflow.md)