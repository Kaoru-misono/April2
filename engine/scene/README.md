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

**Entity**: `uint32_t` with `NullEntity = 0xFFFFFFFF`

**SparseSet<T>**: Component storage
- Dense array for cache-friendly iteration
- Sparse array for O(1) entity-to-component lookup
- Swap-and-pop removal to maintain packed arrays

**Registry**: ECS manager with EnTT-style API
- `create()` - Create entity
- `destroy(Entity)` - Destroy entity and all components
- `emplace<T>(Entity, Args...)` - Add component
- `get<T>(Entity)` - Get component reference
- `all_of<T>(Entity)` - Check if entity has component
- `view<T>()` - Get iterable view of all components

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

// Iterate components
for (auto& transform : registry.view<TransformComponent>())
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

1. **No Entity Recycling**: Simple linear ID generation for now
2. **Linked List Hierarchy**: O(1) add/remove, O(n) traversal
3. **Dirty Flag Transforms**: Manual marking, automatic propagation
4. **Single-Component Views**: Multi-component views not yet implemented
5. **EnTT Compatibility**: Easy migration path to full EnTT library

## Limitations & Future Work

- [ ] Multi-component views (e.g., `view<Transform, Velocity>()`)
- [ ] Entity recycling with generation counters
- [ ] Component events (on_construct, on_destroy)
- [ ] Entity iteration without component filtering
- [ ] Automatic transform update system
- [ ] Component serialization support

## Testing

See `example-usage.cpp` for detailed usage patterns.

## Migration to EnTT

To switch to the real EnTT library:

1. Replace `#include "scene/scene.hpp"` with `#include <entt/entt.hpp>`
2. Replace `april::scene::Registry` with `entt::registry`
3. Replace `april::scene::Entity` with `entt::entity`
4. Update `NullEntity` to `entt::null`

The API is intentionally compatible to minimize changes.
