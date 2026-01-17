# Specification - ImGui Scissor Fix (RHI Integration)

## Overview
This track aims to resolve persistent rendering issues in the ImGui integration, specifically focusing on incorrect scissor/clipping behavior. Previous attempts (track `imgui-fix_20260117`) failed to correctly map ImGui's coordinate system to the April RHI's expectation. This "redo" will involve a rigorous line-by-line comparison with the reference `imgui_impl_vulkan.cpp` while strictly adhering to the April RHI abstraction.

## Functional Requirements
- **Correct Coordinate Transformation:** Accurately calculate scissor rectangles by accounting for `DisplayPos`, `DisplaySize`, and `FramebufferScale`.
- **RHI Compliance:** Map ImGui scissor commands to the equivalent April RHI calls (e.g., `CommandList::setScissorRect`). **Direct Vulkan (`vk...`) calls are strictly prohibited.**
- **High-DPI Support:** Ensure clipping remains correct on displays with non-1.0 scale factors.
- **Origin Alignment:** Verify the mapping between ImGui's top-left origin and the RHI's viewport/render target coordinate system.

## Non-Functional Requirements
- **Performance:** Avoid redundant state changes in the rendering loop.
- **Maintainability:** Document any divergence from the standard `imgui_impl_vulkan.cpp` logic required by the RHI abstraction.

## Acceptance Criteria
- ImGui windows and elements are clipped correctly when overlapping or partially off-screen.
- Resizing the main window or individual ImGui windows does not result in "popping" or disappearing elements.
- Logged scissor rect values match expected bounds calculated from ImGui's internal state.
- The implementation passes manual inspection in the editor environment.

## Out of Scope
- Refactoring the general ImGui resource management (buffers/descriptors) unless directly related to clipping.
- Performance optimization of the broader RHI implementation.