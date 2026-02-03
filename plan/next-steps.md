# Next Steps Plan

## Context
This plan focuses on the `engine` modules and their boundaries, without diving into third-party library details.

## Module Dependencies (from manifests)
- core: base module
- asset: depends on core
- graphics: depends on core, asset
- ui: depends on core, graphics
- runtime: depends on core, graphics, ui
- editor: depends on runtime

## Milestones and Tasks

### Milestone 1: Unified Input System
- [x] Define a core-level input event model (mouse, keyboard, wheel, focus)
- [x] Route GLFW callbacks into the core event/input layer
- [x] Add input state query API for runtime/editor (pressed/down/released)
- [x] Define routing rules between ImGui and in-engine input
- [x] Add minimal input tests (key/mouse state transitions)

### Milestone 2: Scene/World Data Model
- [ ] Choose data model (ECS vs simple scene graph) and document it
- [ ] Implement basic entity lifecycle (create/destroy, transform)
- [ ] Add component storage for renderable meshes
- [ ] Make renderer consume scene data instead of hardcoded demo mesh
- [ ] Add scene serialization format (initial JSON or binary)

### Milestone 3: Rendering Pipeline Structure
- [ ] Define pass graph for forward rendering + overlays
- [ ] Separate scene rendering from UI/composite logic
- [ ] Add stable resource lifecycle management (RTs, depth, transient buffers)
- [ ] Add basic lighting uniforms (single directional light)
- [ ] Validate with a multi-mesh scene

### Milestone 4: Editor Interaction Loop
- [ ] Implement selection system wired to scene entities
- [ ] Add viewport picking (ray cast vs mesh bounds)
- [ ] Add gizmo transforms that update entity transforms
- [ ] Wire Inspector to edit selected entity data
- [ ] Save/load scene from editor UI

### Milestone 5: Asset Pipeline Improvements
- [ ] Plugin-style importer registration
- [ ] DDC versioning and cache invalidation
- [ ] Async asset loading and main-thread handoff
- [ ] Hot-reload on file change
- [ ] Large scene stress test (loading + editing)
