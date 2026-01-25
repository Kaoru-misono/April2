# Refactor: Replace ResourceBindFlags with Usage Enums

## 1. Overview
The current `ResourceBindFlags` enum conflates buffer and texture usage, leading to less type-safe and more ambiguous resource creation. This refactor aims to replace `ResourceBindFlags` with distinct `BufferUsage` and `TextureUsage` enums. This change will enforce stricter type safety, improve code clarity, and align with modern graphics API patterns.

## 2. Functional Requirements

### 2.1 Core Resource Classes
- **Buffer Class:**
    - Update constructors to accept `BufferUsage` instead of `ResourceBindFlags`.
    - Internal members and logic must store and validate `BufferUsage`.
- **Texture Class:**
    - Update constructors to accept `TextureUsage` instead of `ResourceBindFlags`.
    - Internal members and logic must store and validate `TextureUsage`.
- **RenderDevice Class:**
    - Update `createBuffer` and `createTexture` factory methods to accept the respective usage enums.
    - Remove `getFormatBindFlags` or update it to return specific usage types if applicable.

### 2.2 Engine Subsystems (Call Sites)
- **Refactor all usage of `ResourceBindFlags` to the new enums in:**
    - `GpuProfiler` (e.g., `GpuTimer` buffer creation)
    - `ImGui` Layer (e.g., vertex/index buffers)
    - `RHI` Tools and Validation
    - `RenderDevice` implementation
    - `ParameterBlock` validation logic

### 2.3 Tests
- Update all unit tests and integration tests that create resources to use the new API.
- Ensure all tests compile and pass with the new usage flags.

## 3. Technical Strategy
- **Step 1: Header & Implementation Update:** Modify `Buffer` and `Texture` headers/sources first.
- **Step 2: Device API Update:** Update `RenderDevice` to propagate these changes.
- **Step 3: Call Site Refactoring:** Iteratively update subsystems (`GpuProfiler`, `UI`, `Tests`) to fix compilation errors.
- **Step 4: Cleanup:** Remove `ResourceBindFlags` definition once no longer referenced.

## 4. Acceptance Criteria
- `ResourceBindFlags` is completely removed from the codebase.
- `Buffer` creation uses `BufferUsage`.
- `Texture` creation uses `TextureUsage`.
- The project compiles without errors.
- All existing tests pass.

## 5. Out of Scope
- Changing the underlying RHI implementation (Slang-RHI) behavior, other than mapping the new flags to it.
