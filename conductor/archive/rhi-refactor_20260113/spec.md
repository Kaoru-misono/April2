# Specification: Refactor RHI Command Context and Pipelines

## 1. Overview
This track aims to modernize the Render Hardware Interface (RHI) by unifying command recording into a single `CommandContext` and adopting a clear "Pipeline" nomenclature. The specialized context classes (`CopyContext`, `RenderContext`, `ComputeContext`) will be replaced by a robust `CommandContext` that validates operations based on queue capabilities. Additionally, `*StateObject` classes will be replaced by distinct `*Pipeline` classes (Graphics, Compute, RayTracing).

## 2. Functional Requirements

### 2.1 Unified Command Context
-   **Implementation:** Complete the implementation of `CommandContext` to handle all types of command recording (Graphics, Compute, Copy, RayTracing).
-   **Capability Validation:** The `CommandContext` must be aware of the underlying queue's capabilities (e.g., Graphics, Compute, Copy).
    -   It must enforce restrictions, such as preventing draw calls on a Copy-only queue or dispatch calls on a specific queue type if unsupported.
    -   This validation should ideally happen at runtime (or compile time if feasible with the current architecture) to prevent invalid API usage.
-   **Resource Barriers:** The context must manage resource barriers and transitions efficiently within the unified interface.

### 2.2 Pipeline Abstraction
-   **New Classes:** Implement specific pipeline classes to replace the generic "StateObject" pattern:
    -   `GraphicsPipeline` (replaces `GraphicsStateObject`)
    -   `ComputePipeline` (replaces `ComputeStateObject`)
    -   `RayTracingPipeline` (replaces `RtStateObject`)
-   **Functionality:** These classes will encapsulate the necessary state verification and compilation required for their respective pipelines.

### 2.3 Migration & Cleanup
-   **Reference & Removal:** Retain existing `CopyContext`, `RenderContext`, and `ComputeContext` files temporarily during implementation to serve as a reference for feature parity.
-   **Final Deletion:** Once `CommandContext` and the new Pipelines are fully functional and tested, the old Context and StateObject files/classes must be permanently deleted.
-   **Existing Code:** The recently added `CommandContext.hpp` (from commit `d8336ea`) serves as the starting point. It should be refined and extended to meet these requirements.

## 3. Non-Functional Requirements
-   **Performance:** The unification should not introduce significant CPU overhead for command recording.
-   **Maintainability:** The API should be intuitive, reducing the cognitive load of managing multiple context types.

## 4. Acceptance Criteria
-   [ ] `CommandContext` is the sole interface for recording commands.
-   [ ] `GraphicsPipeline`, `ComputePipeline`, and `RayTracingPipeline` are implemented and used.
-   [ ] Attempting to record a command on an incompatible queue (e.g., `Draw` on a `CopyQueue`) results in a validation error/exception.
-   [ ] All legacy `*Context` and `*StateObject` files are removed.
-   [ ] Existing tests (or new tests replacing them) pass successfully using the new architecture.
