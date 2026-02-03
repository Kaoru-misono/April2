#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <utility>
#include <cassert>

namespace april::scene
{
    // Entity type definition
    using Entity = uint32_t;
    inline constexpr Entity NullEntity = 0xFFFFFFFF;

    // Base class for type-erased sparse set storage
    class ISparseSet
    {
    public:
        virtual ~ISparseSet() = default;
        virtual auto remove(Entity e) -> void = 0;
        virtual auto contains(Entity e) const -> bool = 0;
    };

    // Sparse set implementation for component storage
    template<typename T>
    class SparseSet : public ISparseSet
    {
    public:
        SparseSet() = default;

        // Add a component to an entity
        template<typename... Args>
        auto emplace(Entity entity, Args&&... args) -> T&
        {
            // Ensure sparse array is large enough
            if (entity >= m_entityToDense.size())
            {
                m_entityToDense.resize(entity + 1, kInvalidIndex);
            }

            // Entity shouldn't already have this component
            assert(m_entityToDense[entity] == kInvalidIndex && "Entity already has this component");

            // Add to dense arrays
            auto const denseIndex = m_data.size();
            m_data.emplace_back(std::forward<Args>(args)...);
            m_denseToEntity.push_back(entity);
            m_entityToDense[entity] = denseIndex;

            return m_data.back();
        }

        // Remove a component from an entity (swap-and-pop)
        auto remove(Entity entity) -> void override
        {
            if (entity >= m_entityToDense.size() || m_entityToDense[entity] == kInvalidIndex)
            {
                return; // Entity doesn't have this component
            }

            auto const denseIndex = m_entityToDense[entity];
            auto const lastIndex = m_data.size() - 1;

            if (denseIndex != lastIndex)
            {
                // Swap with last element
                m_data[denseIndex] = std::move(m_data[lastIndex]);
                auto const movedEntity = m_denseToEntity[lastIndex];
                m_denseToEntity[denseIndex] = movedEntity;
                m_entityToDense[movedEntity] = denseIndex;
            }

            // Remove last element
            m_data.pop_back();
            m_denseToEntity.pop_back();
            m_entityToDense[entity] = kInvalidIndex;
        }

        // Get component reference
        auto get(Entity entity) -> T&
        {
            assert(entity < m_entityToDense.size() && m_entityToDense[entity] != kInvalidIndex);
            return m_data[m_entityToDense[entity]];
        }

        auto get(Entity entity) const -> T const&
        {
            assert(entity < m_entityToDense.size() && m_entityToDense[entity] != kInvalidIndex);
            return m_data[m_entityToDense[entity]];
        }

        // Check if entity has this component
        auto contains(Entity entity) const -> bool override
        {
            return entity < m_entityToDense.size() && m_entityToDense[entity] != kInvalidIndex;
        }

        // Direct access to dense component array
        auto data() -> std::vector<T>& { return m_data; }
        auto data() const -> std::vector<T> const& { return m_data; }

        // Get entity ID from dense index
        auto getEntity(size_t denseIndex) const -> Entity
        {
            assert(denseIndex < m_denseToEntity.size() && "Dense index out of bounds");
            return m_denseToEntity[denseIndex];
        }

        // Iterator support
        auto begin() { return m_data.begin(); }
        auto end() { return m_data.end(); }
        auto begin() const { return m_data.begin(); }
        auto end() const { return m_data.end(); }

    private:
        static constexpr size_t kInvalidIndex = static_cast<size_t>(-1);

        std::vector<T> m_data;                    // Dense array of components
        std::vector<Entity> m_denseToEntity;      // Map dense index -> Entity ID
        std::vector<size_t> m_entityToDense;      // Map Entity ID -> dense index (sparse array)
    };

    // View proxy for iterating over components
    template<typename T>
    class View
    {
    public:
        explicit View(SparseSet<T>* set) : m_set{set} {}

        auto begin() { return m_set->begin(); }
        auto end() { return m_set->end(); }
        auto begin() const { return m_set->begin(); }
        auto end() const { return m_set->end(); }

    private:
        SparseSet<T>* m_set;
    };

    // Registry - the ECS manager with EnTT-style API
    class Registry
    {
    public:
        Registry() = default;

        // Create a new entity
        auto create() -> Entity
        {
            return ++m_entityCounter;
        }

        // Destroy an entity and remove all its components
        auto destroy(Entity e) -> void
        {
            for (auto& [type, pool] : m_pools)
            {
                pool->remove(e);
            }
        }

        // Add a component to an entity
        template<typename T, typename... Args>
        auto emplace(Entity e, Args&&... args) -> T&
        {
            auto* pool = getOrCreatePool<T>();
            return pool->emplace(e, std::forward<Args>(args)...);
        }

        // Get a component from an entity
        template<typename T>
        auto get(Entity e) -> T&
        {
            auto* pool = getPool<T>();
            assert(pool != nullptr && "Component pool doesn't exist");
            return pool->get(e);
        }

        template<typename T>
        auto get(Entity e) const -> T const&
        {
            auto const* pool = getPool<T>();
            assert(pool != nullptr && "Component pool doesn't exist");
            return pool->get(e);
        }

        // Check if entity has a component
        template<typename T>
        auto allOf(Entity e) const -> bool
        {
            auto const* pool = getPool<T>();
            return pool != nullptr && pool->contains(e);
        }

        // Get a view for iterating over all components of type T
        template<typename T>
        auto view() -> View<T>
        {
            auto* pool = getOrCreatePool<T>();
            return View<T>{pool};
        }

        // Get pool for manual iteration
        template<typename T>
        auto getPool() const -> SparseSet<T> const*
        {
            auto const typeIndex = std::type_index{typeid(T)};
            auto it = m_pools.find(typeIndex);
            return it != m_pools.end() ? static_cast<SparseSet<T> const*>(it->second.get()) : nullptr;
        }

        template<typename T>
        auto getPool() -> SparseSet<T>*
        {
            return const_cast<SparseSet<T>*>(static_cast<Registry const*>(this)->getPool<T>());
        }

    private:
        // Get or create a sparse set pool for component type T
        template<typename T>
        auto getOrCreatePool() -> SparseSet<T>*
        {
            auto* pool = getPool<T>();
            if (pool)
            {
                return pool;
            }

            auto const typeIndex = std::type_index{typeid(T)};
            auto newPool = std::make_unique<SparseSet<T>>();
            auto* poolPtr = newPool.get();
            m_pools[typeIndex] = std::move(newPool);
            return poolPtr;
        }

        std::unordered_map<std::type_index, std::unique_ptr<ISparseSet>> m_pools;
        uint32_t m_entityCounter = 0;
    };

} // namespace april::scene
