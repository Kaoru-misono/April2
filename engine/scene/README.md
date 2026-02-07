# Scene Module - Lightweight ECS Implementation

A custom Entity Component System (ECS) implementation with EnTT-compatible API for the April2 engine.

## Features

- **EnTT-Compatible API**: Designed to allow easy migration to/from the EnTT library
- **Sparse Set Storage**: O(1) component lookups with cache-friendly dense arrays
- **Scene Graph**: Hierarchical entity relationships with linked-list structure
- **Transform System**: Automatic world matrix computation with dirty flagging
- **C++23**: Modern C++ with trailing return types, east const, and auto

## Components

### Core ECS (`ecs-core.hpp`)

**Entity**: handle with index + generation
```cpp
struct Entity { uint32_t index; uint32_t generation; };
inline constexpr Entity NullEntity{ 0xFFFFFFFF, 0 };
```

**SparseSet<T>**: Component storage
- Dense array for cache-friendly iteration
- Sparse array for O(1) entity-to-component lookup
- Swap-and-pop removal to maintain packed arrays

**Registry**: ECS manager with EnTT-style API
- `create()` - Create entity
- `destroy(Entity)` - Destroy entity and all components
- `emplace<T>(Entity, Args...)` - Add component
- `get<T>(Entity)` - Get component reference
- `allOf<T>(Entity)` - Check if entity has component
- `view<T>()` - Get iterable view of all components
- `view<A, B, ...>()` - Multi-component view with intersection semantics

### Standard Components (`components.hpp`)

- **IDComponent**: Unique UUID for each entity
- **TagComponent**: Human-readable name/tag
- **TransformComponent**: Local transform (position, rotation, scale) and world matrix
- **RelationshipComponent**: Parent-child hierarchy using linked lists

### Scene Graph (`scene-graph.hpp/cpp`)

**SceneGraph**: High-level scene management
- `createEntity(name)` - Create entity with core components
- `destroyEntity(entity)` - Recursively destroy entity and children
- `setParent(child, parent)` - Manage hierarchy relationships
- `updateTransform(entity, parentMatrix)` - Recursive transform propagation
- `getRegistry()` - Access underlying ECS registry

## Usage Example

```cpp
#include "scene/scene.hpp"

using namespace april::scene;

// Create scene
SceneGraph scene{};

// Create entities
auto root = scene.createEntity("Root");
auto child = scene.createEntity("Child");

// Access registry for component operations
auto& registry = scene.getRegistry();

// Modify components
auto& transform = registry.get<TransformComponent>(root);
transform.localPosition = float3{0.f, 1.f, 0.f};
transform.localRotation = float3{0.f, glm::radians(45.f), 0.f};

// Set up hierarchy
scene.setParent(child, root);

// Iterate components with entity handles
for (auto [entity, transform] : registry.view<TransformComponent>().each())
{
    transform.isDirty = true;
}

// Add custom components
struct VelocityComponent { float3 velocity; };
registry.emplace<VelocityComponent>(child, float3{1.f, 0.f, 0.f});

// Clean up (recursive - destroys all children)
scene.destroyEntity(root);
```

## Architecture

### Sparse Set Data Structure

```
Entity IDs:     [  5,  2, 17,  9 ]  (scattered)
                 ↓   ↓   ↓   ↓
m_entityToDense: [-, -, 1, -, -, 0, -, -, -, 3, -, -, -, -, -, -, -, 2]
                         ↓     ↓           ↓                 ↓
                         ↓     ↓           ↓                 ↓
m_data:          [comp5, comp2, comp17, comp9]  (packed, cache-friendly)
m_denseToEntity: [  5,    2,    17,     9  ]
```

- **m_entityToDense**: Sparse array (Entity ID → dense index)
- **m_data**: Dense array of components (for iteration)
- **m_denseToEntity**: Reverse mapping (dense index → Entity ID)

### Hierarchy Structure

Using intrusive linked lists for parent-child relationships:

```
Parent
├─ firstChild → Child1
│               ├─ nextSibling → Child2
│               │                 ├─ nextSibling → Child3
│               │                 └─ prevSibling → Child1
│               └─ prevSibling → null
```

Each entity has:
- `parent`: Parent entity ID
- `firstChild`: First child in linked list
- `prevSibling`: Previous sibling
- `nextSibling`: Next sibling
- `childrenCount`: Number of direct children

## Design Decisions

1. **Entity Recycling**: Free list + generation counters for safe reuse
2. **Linked List Hierarchy**: O(1) add/remove, O(n) traversal
3. **Dirty Flag Transforms**: Manual marking, automatic propagation
4. **Multi-Component Views**: Intersection-based iteration, driven by smallest pool
5. **EnTT Compatibility**: Easy migration path to full EnTT library

## Limitations & Future Work

- [ ] Entity iteration without component filtering
- [ ] Component events (on_construct, on_destroy)
- [ ] Automatic transform update system
- [ ] Component serialization support

## Testing

See `example-usage.cpp` for detailed usage patterns.

## Migration to EnTT

To switch to the real EnTT library:

1. Replace `#include "scene/scene.hpp"` with `#include <entt/entt.hpp>`
2. Replace `april::scene::Registry` with `entt::registry`
3. Replace `april::scene::Entity` with `entt::entity` and remove any `.index/.generation` accessors
4. Update `NullEntity` to `entt::null`

The API is intentionally compatible to minimize changes.


## Plan (2026-02-07): SceneGraph + SparseSet ECS Upgrade (Step-by-step)

This document turns the recommended changes into a concrete engineering checklist.
Goal: evolve the current `SceneGraph + SparseSet` ECS into a scalable, runtime-friendly, render-friendly scene system **without rewriting everything**.
Editor-facing integrations are intentionally out of scope for this module and will live in `sceneSubSystem`.
Status: Completed on 2026-02-07.

---

### 0. Current Pain Points (Why changes are required)

#### 0.1 Views cannot yield `Entity`
Your current `View<T>` iterates only components (`T&`) and does not expose the owning `Entity`.
This blocks:
- Writing real systems (`TransformSystem`, `RenderExtract`, `PhysicsSync`, etc.)
- Iterating `Transform + Relationship` together
- Efficient scene update pipelines

#### 0.2 No entity reuse (IDs grow forever)
`Registry::create()` always increments `m_entityCounter`. Deleted entities are never recycled.
This causes:
- Entity IDs to increase without bound
- Each `SparseSet` sparse array (`m_entityToDense`) to grow huge due to `resize(entity + 1)`
- Memory blow-ups over time (especially in long-running sessions)

#### 0.3 Root detection is O(N) scan each frame
`updateTransforms()` scans the entire `RelationshipComponent` pool to find roots (`parent == NullEntity`).
This is functional but wastes time for large scenes and makes streaming harder.

#### 0.4 No circular dependency check in parenting
`setParent()` has no cycle detection. Runtime reparenting can create loops and break recursion.

---

### 1. ECS Foundation: Make Views usable by Systems

#### 1.1 Upgrade `View<T>` to iterate entities (minimum change)
Add an iterator that walks the dense array by index, and yields:
- `Entity e`
- `T& component`

Recommended API:
- Range-for: `for (auto [e, t] : registry.view<T>().each())`
- Callback: `registry.view<T>().each([](Entity e, T& t){ ... });`

Implementation notes:
- Use `SparseSet<T>::getEntity(denseIndex)` to get `Entity`
- Use `SparseSet<T>::data()[denseIndex]` to get the component

Deliverables:
- `View<T>::each()` range wrapper or iterator
- `View<T>::each(fn)` callback method

---

#### 1.2 Add multi-component views (`view<A, B, ...>`)
Implement `view<Ts...>` with intersection semantics.

Recommended algorithm (fast enough, minimal complexity):
1. Pick the smallest dense pool as the “driver”
2. For each entity in the driver pool:
   - Check `contains(e)` in all other pools
   - If all true, yield `(e, get<Ts>(e)...)`

Deliverables:
- `Registry::view<A, B>()`
- `Registry::view<A, B, C>()` (variadic)
- Range-for iteration yielding tuples `(Entity, A&, B& ...)`

Benefits:
- `Transform + Relationship` systems become clean and fast
- Rendering extract can iterate `Transform + MeshRenderer`
- Physics sync can iterate `RigidBody + Transform`

---

### 2. Entity Lifecycle: Prevent memory blow-ups

#### 2.1 Add free list recycling (minimum)
Change entity creation to reuse destroyed IDs.

Implementation:
- `std::vector<Entity> m_free;`
- `create()` pops from `m_free` if available, else increments counter
- `destroy(e)` removes components then pushes `e` into `m_free`

Deliverables:
- Entity reuse enabled
- Avoid unbounded sparse array growth

⚠️ Warning: free-list alone can create “dangling entity handle” bugs if old IDs are stored elsewhere.

---

#### 2.2 Add generation counters (recommended for runtime safety)
Move from `using Entity = uint32_t;` to a handle:
```cpp
struct Entity { uint32_t index; uint32_t gen; };
Registry maintains:

std::vector<uint32_t> m_generation;

std::vector<uint32_t> m_freeIndices;
```

Rules:

create() returns { index, gen }

destroy() increments generation of the index, recycles index

valid(e) checks m_generation[e.index] == e.gen

SparseSet changes:

Use e.index as the sparse key

denseToEntity stores full Entity handle (or index only if generation validated elsewhere)

Deliverables:

Registry::valid(Entity e)

Updated SparseSet to use indices safely

Prevent use-after-free entity bugs

### 3. SceneGraph: Make hierarchy + transform updates scalable
#### 3.1 Add parent cycle detection in setParent()

Reject parenting if newParent is inside child's subtree.

Efficient check:

Walk upward from newParent using RelationshipComponent.parent

If you reach child, reject (cycle)

Deliverables:

bool isDescendant(Entity maybeParent, Entity childRoot)

Use it in setParent(child, newParent)

#### 3.2 Maintain root entity list (avoid scanning all relationships each frame)

Add:

std::vector<Entity> m_roots; (or unordered_set)

Rules:

On create: entity starts as root

On setParent:

If old parent was Null and new parent is non-null => remove from roots

If new parent is Null => add to roots

Then updateTransforms() becomes:

For each root: call updateTransform(root, identity, false)

Deliverables:

Root tracking data structure

Updated setParent() to maintain roots

Faster transform update for large scenes

#### 3.3 Optional: Track dirty roots for partial transform updates (high ROI for runtime)

Instead of updating all roots every frame:

Keep m_dirtyRoots set/list

When a node becomes dirty, insert it unless an ancestor is already dirty

Update only dirty roots each frame, then clear list

Use cases:

Gameplay-driven transforms update only the affected subtree

Reparent updates only the moved subtree

Deliverables:

markTransformDirty(Entity e) + dirty root promotion logic

Update pipeline uses m_dirtyRoots (fallback to roots if empty)

### 4. Camera system: keep current behavior, add future-proofing

#### 4.1 Keep view matrix = inverse(worldMatrix)

This is okay. Ensure it is run after transform updates.

#### 4.2 Lock down math conventions (important)

Make sure your engine defines:

Right-hand vs Left-hand coordinates

NDC depth range: [-1, 1] vs [0, 1]

GLM compile-time macros (if using Vulkan / D3D)

Deliverables:

A central "Math Conventions" header or configuration

Ensure glm::perspective/ortho aligns with the chosen conventions

### 5. Render-friendly evolution path (minimal but crucial structure)

Even if you stay single-threaded initially, define the separation:

World (ECS runtime data)

RenderWorld (render proxies)

Extract() step copies data from World to RenderWorld

#### 5.1 Add a basic Extract stage

Example:

Iterate Transform + MeshRenderer

Write RenderableProxy into RenderWorld

Deliverables:

RenderWorld::upsert(Entity e, RenderableProxy p)

extractRenderables(Registry const&, RenderWorld&)

Avoid rendering directly from ECS data later (enables multithreading + double buffering)

### 6. Priority Order (recommended execution)

View<T> yields Entity

Multi-component views

Entity recycling (free list), ideally generation

SceneGraph cycle detection

SceneGraph root tracking

Dirty roots optimization

RenderWorld + Extract stage skeleton

### 7. Acceptance Criteria (Definition of Done)

#### ECS

Can iterate (Entity, Component&) for single component view

Can iterate (Entity, A&, B& ...) for multi-component view

Entity IDs do not grow unbounded during create/destroy churn

(If generation enabled) invalid entities are detected safely

#### SceneGraph

Reparenting cannot create cycles

Transform update does not require scanning all relationships each frame

Dirty subtree updates are possible (optional but recommended)


## Plan (2026-02-07): Double-Buffered Frame Snapshot Architecture
Status: Completed on 2026-02-07.

### 1. Context & Objective

**Current State**
- The engine currently operates with a tightly coupled ECS, SceneGraph, and SceneRenderer.
- The SceneRenderer directly accesses ECS components or SceneGraph nodes, which is not thread-safe and hinders future multi-threading.

**Goal**
- Refactor the rendering pipeline to decouple Game Logic (World) from Rendering (Scene).
- Implement a Frame Snapshot architecture (similar to Stingray/Godot).
- Do NOT implement a Render Graph (RDG) yet. Focus solely on the data flow and extraction mechanism.
- The architecture must support future multi-threading (Game Thread producing data, Render Thread consuming data).

### 2. Architecture Overview

Pipeline flow:
1. Game Thread (Update): ECS systems update logic & transforms.
2. Game Thread (Extraction): a RenderExtractionSystem copies relevant rendering data from ECS into a FrameSnapshot.
3. Sync Point: the filled snapshot is submitted to the Render Thread (or swapped in a single-threaded test).
4. Render Thread (Render): the SceneRenderer reads only from the FrameSnapshot to execute draw calls.

### 3. Implementation Steps

#### Step 1: Define Data Contracts (`render-types.hpp`)
Create a header to define the protocol between Logic and Rendering. These structures must be POD or simple containers.

Requirements:
- Define RenderID (`uint32_t`) to decouple logic assets from GPU resources.
- Define MeshInstance and LightInstance as flat structs.
- Define ViewSnapshot for camera data.
- Define FrameSnapshot.

Example schema:
```cpp
using RenderID = uint32_t;
constexpr RenderID kInvalidRenderID = 0;

struct MeshInstance {
    Matrix4x4 WorldTransform;
    RenderID MeshID;      // Handle to GPU Geometry
    RenderID MaterialID;  // Handle to GPU Pipeline/Material
    AABB WorldBounds;     // For Frustum Culling
};

struct LightInstance {
    Vector3 Position;
    Vector3 Color;
    float Radius;
    float Intensity;
};

struct ViewSnapshot {
    Matrix4x4 ViewMatrix;
    Matrix4x4 ProjectionMatrix;
    Vector3 CameraPosition;
};

struct FrameSnapshot {
    ViewSnapshot MainView;
    std::vector<MeshInstance> StaticMeshes;
    std::vector<MeshInstance> DynamicMeshes;
    std::vector<LightInstance> Lights;

    void Reset() {
        StaticMeshes.clear();
        DynamicMeshes.clear();
        Lights.clear();
        // Keep capacity to minimize allocations
    }
};
```

#### Step 2: Resource Handle Management
The Renderer needs a way to look up actual GPU resources using the RenderID from the snapshot.

Task:
- Create a simple RenderResourceManager or modify the existing Renderer to maintain a registry.

Mechanism:
- `std::vector<GPUBuffer*>` meshes indexed by RenderID.
- `std::vector<GPUPipeline*>` materials indexed by RenderID.

Action:
- Update the ECS MeshComponent to store a RenderID instead of a raw pointer to a Model/Asset.

#### Step 3: Implement Extraction System (`render-extraction.hpp`)
Create a new ECS system responsible for populating the snapshot.

Execution order: run AFTER SceneGraphSystem (world transforms calculated) and BEFORE Renderer::Render.

Logic:
1. Acquire a write snapshot (cleared).
2. Query all entities with MeshComponent and TransformComponent.
3. Copy WorldMatrix from Transform and RenderID from MeshComponent into `FrameSnapshot::DynamicMeshes`.
4. Extract camera data into `FrameSnapshot::MainView`.
5. Submit the snapshot.

#### Step 4: Refactor SceneRenderer
Modify the existing SceneRenderer to stop reading ECS/SceneGraph data directly.

New signature:
`void render(const FrameSnapshot& snapshot)`

Logic:
1. Setup view uniforms using `snapshot.mainView`.
2. Iterate `snapshot.dynamicMeshes`.
3. Retrieve GPU resources using `MeshInstance.meshId`.
4. Issue draw calls.

### 4. Technical Constraints & Guidelines

- No logic leakage: the SceneRenderer must not include ECS headers or access Entity objects. It only knows about FrameSnapshot.
- Memory management: ensure FrameSnapshot reuses memory (clears but keeps capacity).
- Transform precision: ensure the WorldMatrix passed to the snapshot is the final world-space transform.
- Language: C++23 or newer.

### 5. Definition of Done

- The project compiles.
- The SceneRenderer no longer depends on ECS/SceneGraph headers.
- A scene is rendered using data solely populated via the RenderExtractionSystem.
- Moving an Entity in ECS correctly updates the rendered position on the next frame via the snapshot mechanism.
