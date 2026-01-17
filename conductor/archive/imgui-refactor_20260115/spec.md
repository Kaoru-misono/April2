# Specification: ImGui Layer RHI Refactor and Validation

## 1. Overview
The goal of this track is to refactor the `ImGuiLayer` module to fully utilize the engine's RHI (Render Hardware Interface) abstraction. This will replace any remaining direct API calls (D3D12/Vulkan) within `imgui-layer.cpp` and `imgui-layer.hpp` with RHI-compliant code. Additionally, a dedicated testing suite will be implemented to ensure the reliability and visual correctness of the UI layer.

## 2. Functional Requirements

### 2.1 RHI Integration
- **Backend Refactor:** Re-implement the ImGui rendering backend using `april::graphics` RHI objects (Buffers, Textures, PSOs, CommandEncoders).
- **Resource Management:**
    - The `ImGuiLayer` will manage its own vertex/index buffers and font textures using RHI handles.
    - Common states (like samplers) will be retrieved from the global RHI state where appropriate (Hybrid approach).
- **Platform Agnosticism:** Ensure the refactored layer works seamlessly across all RHI backends (D3D12, Vulkan) without conditional compilation for specific APIs inside the UI module.

### 2.2 Testing & Validation
- **Dedicated Test Executable:** Create `test-imgui` to host UI-specific tests.
- **Unit Tests:**
    - Verify RHI resource creation (Buffers, Textures, PSOs) specific to ImGui requirements.
    - Validate resource lifecycle (allocation, updates, and cleanup) for dynamic UI data.
- **Visual Validation:**
    - Implement a minimal rendering test that verifies the ImGui overlay appears correctly.
    - Test font texture sampling and clipping rect functionality.

## 3. Non-Functional Requirements
- **Performance:** Minimize CPU-GPU synchronization when updating UI vertex/index buffers.
- **Maintainability:** Align `ImGuiLayer` implementation with the project's established coding standards and RHI usage patterns.

## 4. Acceptance Criteria
- [ ] `imgui-layer.cpp` and `imgui-layer.hpp` are free of direct D3D12/Vulkan calls.
- [ ] `test-imgui` executable is created and integrated into the build system.
- [ ] Unit tests for ImGui RHI resources pass on all supported backends.
- [ ] Visual verification confirms that ImGui renders correctly (text, buttons, windows).
- [ ] No regressions in existing engine functionality.

## 5. Out of Scope
- Porting ImGui to non-RHI platforms.
- Implementing high-level UI widgets (focus is on the backend/layer implementation).
