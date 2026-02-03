I need to implement a **Lightweight, Custom ECS** for my engine "April2" in C++23.
**Crucial Requirement:** The API must mimic the **EnTT** library's API style so that I can easily swap this custom implementation for the real EnTT library in the future without rewriting game logic.

Please implement this in `namespace april::scene` in the scene module.

### 1. The Core ECS (`ecs-core.hpp`)

**Entity:**
- Define `using Entity = uint32_t;`
- Define `Entity const NullEntity = 0xFFFFFFFF;`

**Sparse Set Storage (The Engine):**
- Create a base class `ISparseSet` with a virtual destructor and `auto remove(Entity e) -> void`.
- Create a template class `SparseSet<T> : public ISparseSet`.
- Members:
    - `std::vector<T> m_data`: The dense array of components.
    - `std::vector<Entity> m_denseToEntity`: Map dense index -> Entity ID (for reverse lookup).
    - `std::vector<size_t> m_entityToDense`: Map Entity ID -> dense index (Sparse array).
- **Logic**:
    - `emplace(entity, args...)`: Add component to `m_data`, map entity to index.
    - `remove(entity)`: Implement **Swap-and-Pop**. Move the *last* element in `m_data` to the slot of the removed element to keep the array packed. Update maps accordingly.
    - `get(entity)`: Return reference to `T`.
    - `contains(entity)`: Return bool.

**Registry (The Manager):**
- Class `Registry`.
- Members:
    - `std::unordered_map<std::type_index, std::unique_ptr<ISparseSet>> m_pools`: Stores the sparse sets.
    - `uint32_t m_entityCounter = 0`: Simple linear ID generation (no recycling needed for now).
- **API (Must match EnTT style):**
    - `Entity create()`: Returns ++m_entityCounter.
    - `auto destroy(Entity e) -> void`: Iterates all pools and removes `e`.
    - `template<typename T, typename... Args> auto emplace(Entity e, Args&&... args) -> T&`:
        - Get or create `SparseSet<T>`.
        - Forward args to construct T in place.
    - `template<typename T> auto get(Entity e) -> T&`: Returns component.
    - `template<typename T> auto all_of(Entity e) -> bool`: Check if entity has component T.
    - `template<typename T> auto view()`:
        - Should return a lightweight proxy object that has `begin()` and `end()` iterators pointing to `SparseSet<T>::m_data`.
        - This allows `for(auto& component : registry.view<T>())` loops.

### 2. The Components (`components.hpp`)

Implement standard POD structs:
- `IDComponent`: `core::UUID id;`
- `TagComponent`: `std::string tag;`
- `TransformComponent`:
    - `float3 localPosition{0.f}`, `localRotation{0.f}` (Euler), `localScale{1.f}`.
    - `float4x4 worldMatrix{1.f}`.
    - `bool isDirty = true`.
- **RelationshipComponent**:
    - `size_t childrenCount = 0;`
    - `Entity parent = NullEntity;`
    - `Entity firstChild = NullEntity;`
    - `Entity prevSibling = NullEntity;`
    - `Entity nextSibling = NullEntity;`

### 3. The Scene Graph Logic (`scene-graph.hpp` / `scene-graph.cpp`)

Class `SceneGraph`:
- Member: `Registry m_registry`.
- **Functions:**
    - `Entity createEntity(std::string name)`: Creates entity, adds ID, Tag, Transform, and Relationship components.
    - `auto destroyEntity(Entity e) -> void`:
        - **Recursive**: Must destroy all children of `e` first (traverse `firstChild` -> `nextSibling`).
        - Remove from parent's list if `e` has a parent.
        - Call `m_registry.destroy(e)`.
    - `auto setParent(Entity child, Entity newParent) -> void`:
        - **Unlink**: If `child` has a parent, remove `child` from the old parent's linked list (fix prev/next/first ptrs).
        - **Link**: Add `child` to `newParent`'s linked list.
        - Mark transform dirty.

- **Transform System:**
    - `auto onUpdateRuntime(float dt) -> void`:
        - Iterate all entities. If an entity is a root (parent == Null), call `updateTransform(e, identityMatrix)`.
    - `auto updateTransform(Entity e, const glm::mat4& parentMat) -> void`:
        - Update `worldMatrix` if dirty.
        - Recurse to children.

### Constraint
- Use C++23 features (concepts if helpful, but keep it simple).
- Ensure `SparseSet` lookup is O(1).
- Ensure `RelationshipComponent` logic handles linked-list edge cases correctly (e.g., removing the first child, or a middle child).
