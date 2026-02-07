#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <utility>
#include <cassert>
#include <tuple>
#include <type_traits>
#include <limits>
#include <algorithm>
#include <array>
#include <iterator>

namespace april::scene
{
    template<typename T>
    class SparseSet;

    struct Entity
    {
        uint32_t index{std::numeric_limits<uint32_t>::max()};
        uint32_t generation{0};
    };

    inline constexpr uint32_t kInvalidEntityIndex = std::numeric_limits<uint32_t>::max();
    inline constexpr Entity NullEntity{kInvalidEntityIndex, 0};

    [[nodiscard]] auto operator==(Entity lhs, Entity rhs) -> bool;
    [[nodiscard]] auto operator!=(Entity lhs, Entity rhs) -> bool;
    [[nodiscard]] auto isNull(Entity e) -> bool;
    [[nodiscard]] auto toIntegral(Entity e) -> uint64_t;

    struct EntityHash
    {
        auto operator()(Entity e) const noexcept -> size_t;
    };

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
            assert(!isNull(entity) && "Cannot emplace component on NullEntity");

            auto const index = static_cast<size_t>(entity.index);

            // Ensure sparse array is large enough
            if (index >= m_entityToDense.size())
            {
                m_entityToDense.resize(index + 1, kInvalidIndex);
            }

            // Entity shouldn't already have this component
            assert(m_entityToDense[index] == kInvalidIndex && "Entity already has this component");

            // Add to dense arrays
            auto const denseIndex = m_data.size();
            m_data.emplace_back(std::forward<Args>(args)...);
            m_denseToEntity.push_back(entity);
            m_entityToDense[index] = denseIndex;

            return m_data.back();
        }

        // Remove a component from an entity (swap-and-pop)
        auto remove(Entity entity) -> void override
        {
            if (!contains(entity))
            {
                return; // Entity doesn't have this component
            }

            auto const index = static_cast<size_t>(entity.index);
            auto const denseIndex = m_entityToDense[index];
            auto const lastIndex = m_data.size() - 1;

            if (denseIndex != lastIndex)
            {
                // Swap with last element
                m_data[denseIndex] = std::move(m_data[lastIndex]);
                auto const movedEntity = m_denseToEntity[lastIndex];
                m_denseToEntity[denseIndex] = movedEntity;
                m_entityToDense[movedEntity.index] = denseIndex;
            }

            // Remove last element
            m_data.pop_back();
            m_denseToEntity.pop_back();
            m_entityToDense[index] = kInvalidIndex;
        }

        // Get component reference
        auto get(Entity entity) -> T&
        {
            assert(contains(entity));
            return m_data[m_entityToDense[entity.index]];
        }

        auto get(Entity entity) const -> T const&
        {
            assert(contains(entity));
            return m_data[m_entityToDense[entity.index]];
        }

        // Check if entity has this component
        auto contains(Entity entity) const -> bool override
        {
            if (isNull(entity))
            {
                return false;
            }

            auto const index = static_cast<size_t>(entity.index);
            if (index >= m_entityToDense.size())
            {
                return false;
            }

            auto const denseIndex = m_entityToDense[index];
            if (denseIndex == kInvalidIndex)
            {
                return false;
            }

            return m_denseToEntity[denseIndex] == entity;
        }

        // Direct access to dense component array
        auto data() -> std::vector<T>& { return m_data; }
        auto data() const -> std::vector<T> const& { return m_data; }
        auto size() const -> size_t { return m_data.size(); }

        // Get entity handle from dense index
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
        static constexpr size_t kInvalidIndex = std::numeric_limits<size_t>::max();

        std::vector<T> m_data;               // Dense array of components
        std::vector<Entity> m_denseToEntity; // Map dense index -> Entity
        std::vector<size_t> m_entityToDense; // Map Entity index -> dense index (sparse array)
    };

    namespace detail
    {
        template<typename T>
        using SparseSetType = SparseSet<std::remove_const_t<T>>;

        template<typename T>
        using SparseSetPtr = std::conditional_t<std::is_const_v<T>, SparseSetType<T> const*, SparseSetType<T>*>;
    }

    // View proxy for iterating over components
    template<typename T>
    class View
    {
    public:
        using Component = std::remove_const_t<T>;
        using PoolPtr = detail::SparseSetPtr<T>;
        using ComponentRef = std::add_lvalue_reference_t<T>;

        explicit View(PoolPtr set) : m_set{set} {}

        auto begin() { return m_set->begin(); }
        auto end() { return m_set->end(); }
        auto begin() const { return m_set->begin(); }
        auto end() const { return m_set->end(); }

        class EachRange
        {
        public:
            explicit EachRange(PoolPtr set) : m_set(set) {}

            class Iterator
            {
            public:
                using difference_type = std::ptrdiff_t;
                using value_type = std::pair<Entity, ComponentRef>;
                using iterator_category = std::forward_iterator_tag;

                Iterator(size_t index, PoolPtr set)
                    : m_index(index)
                    , m_set(set)
                {
                }

                auto operator*() const -> value_type
                {
                    return value_type{m_set->getEntity(m_index), m_set->data()[m_index]};
                }

                auto operator++() -> Iterator&
                {
                    ++m_index;
                    return *this;
                }

                auto operator==(Iterator const& other) const -> bool
                {
                    return m_index == other.m_index && m_set == other.m_set;
                }

                auto operator!=(Iterator const& other) const -> bool
                {
                    return !(*this == other);
                }

            private:
                size_t m_index{};
                PoolPtr m_set{};
            };

            auto begin() const -> Iterator
            {
                return Iterator{0, m_set};
            }

            auto end() const -> Iterator
            {
                auto const size = m_set ? m_set->size() : 0;
                return Iterator{size, m_set};
            }

        private:
            PoolPtr m_set{};
        };

        auto each() -> EachRange { return EachRange{m_set}; }
        auto each() const -> EachRange { return EachRange{m_set}; }

        template<typename Func>
        auto each(Func&& fn) -> void
        {
            for (auto [entity, component] : each())
            {
                fn(entity, component);
            }
        }

        template<typename Func>
        auto each(Func&& fn) const -> void
        {
            for (auto [entity, component] : each())
            {
                fn(entity, component);
            }
        }

    private:
        PoolPtr m_set{};
    };

    template<typename... Ts>
    class MultiView
    {
    public:
        using Pools = std::tuple<detail::SparseSetPtr<Ts>...>;
        using Tuple = std::tuple<Entity, std::add_lvalue_reference_t<Ts>...>;

        explicit MultiView(detail::SparseSetPtr<Ts>... pools)
            : m_pools(pools...)
        {
        }

        class EachRange
        {
        public:
            explicit EachRange(Pools pools)
                : m_pools(std::move(pools))
            {
                selectDriver();
            }

            class Iterator
            {
            public:
                using difference_type = std::ptrdiff_t;
                using value_type = Tuple;
                using iterator_category = std::forward_iterator_tag;

                Iterator(size_t index, EachRange const* range)
                    : m_index(index)
                    , m_range(range)
                {
                    advanceToValid();
                }

                auto operator*() const -> value_type
                {
                    auto const entity = m_range->getEntityAt(m_index);
                    return m_range->makeTuple(entity);
                }

                auto operator++() -> Iterator&
                {
                    ++m_index;
                    advanceToValid();
                    return *this;
                }

                auto operator==(Iterator const& other) const -> bool
                {
                    return m_index == other.m_index && m_range == other.m_range;
                }

                auto operator!=(Iterator const& other) const -> bool
                {
                    return !(*this == other);
                }

            private:
                auto advanceToValid() -> void
                {
                    while (m_index < m_range->m_driverSize)
                    {
                        auto const entity = m_range->getEntityAt(m_index);
                        if (m_range->containsAll(entity))
                        {
                            break;
                        }
                        ++m_index;
                    }
                }

                size_t m_index{};
                EachRange const* m_range{};
            };

            auto begin() const -> Iterator { return Iterator{0, this}; }
            auto end() const -> Iterator { return Iterator{m_driverSize, this}; }

        private:
            auto selectDriver() -> void
            {
                auto const sizes = collectSizes(std::index_sequence_for<Ts...>{});
                for (size_t i = 0; i < sizes.size(); ++i)
                {
                    if (sizes[i] == 0)
                    {
                        m_driverIndex = i;
                        m_driverSize = 0;
                        return;
                    }
                }

                auto const it = std::min_element(sizes.begin(), sizes.end());
                m_driverIndex = static_cast<size_t>(std::distance(sizes.begin(), it));
                m_driverSize = sizes[m_driverIndex];
            }

            template<size_t... Is>
            auto collectSizes(std::index_sequence<Is...>) const -> std::array<size_t, sizeof...(Ts)>
            {
                return {((std::get<Is>(m_pools) != nullptr) ? std::get<Is>(m_pools)->size() : 0)...};
            }

            template<size_t... Is>
            auto containsAllImpl(Entity entity, std::index_sequence<Is...>) const -> bool
            {
                return (... && (std::get<Is>(m_pools) != nullptr && std::get<Is>(m_pools)->contains(entity)));
            }

            auto containsAll(Entity entity) const -> bool
            {
                return containsAllImpl(entity, std::index_sequence_for<Ts...>{});
            }

            template<size_t... Is>
            auto makeTupleImpl(Entity entity, std::index_sequence<Is...>) const -> Tuple
            {
                return Tuple{entity, (std::get<Is>(m_pools)->get(entity))...};
            }

            auto makeTuple(Entity entity) const -> Tuple
            {
                return makeTupleImpl(entity, std::index_sequence_for<Ts...>{});
            }

            template<size_t I = 0>
            auto getEntityAt(size_t denseIndex) const -> Entity
            {
                if constexpr (I < sizeof...(Ts))
                {
                    if (m_driverIndex == I)
                    {
                        auto* pool = std::get<I>(m_pools);
                        return pool ? pool->getEntity(denseIndex) : NullEntity;
                    }
                    return getEntityAt<I + 1>(denseIndex);
                }
                return NullEntity;
            }

            Pools m_pools{};
            size_t m_driverIndex{0};
            size_t m_driverSize{0};
            friend class Iterator;
        };

        auto each() -> EachRange { return EachRange{m_pools}; }
        auto each() const -> EachRange { return EachRange{m_pools}; }

        template<typename Func>
        auto each(Func&& fn) -> void
        {
            for (auto&& tuple : each())
            {
                std::apply([&](Entity entity, auto&... comps) { fn(entity, comps...); }, tuple);
            }
        }

        template<typename Func>
        auto each(Func&& fn) const -> void
        {
            for (auto&& tuple : each())
            {
                std::apply([&](Entity entity, auto&... comps) { fn(entity, comps...); }, tuple);
            }
        }

    private:
        Pools m_pools{};
    };

    // Registry - the ECS manager with EnTT-style API
    class Registry
    {
    public:
        Registry() = default;

        // Create a new entity
        auto create() -> Entity;

        // Destroy an entity and remove all its components
        auto destroy(Entity e) -> void;

        [[nodiscard]] auto valid(Entity e) const -> bool;

        // Add a component to an entity
        template<typename T, typename... Args>
        auto emplace(Entity e, Args&&... args) -> std::remove_const_t<T>&
        {
            assert(valid(e) && "Cannot emplace component on invalid entity");
            auto* pool = getOrCreatePool<T>();
            return pool->emplace(e, std::forward<Args>(args)...);
        }

        // Get a component from an entity
        template<typename T>
        auto get(Entity e) -> std::remove_const_t<T>&
        {
            auto* pool = getPool<T>();
            assert(pool != nullptr && "Component pool doesn't exist");
            return pool->get(e);
        }

        template<typename T>
        auto get(Entity e) const -> std::remove_const_t<T> const&
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
            return valid(e) && pool != nullptr && pool->contains(e);
        }

        template<typename T, typename... Ts>
            requires (sizeof...(Ts) > 0)
        auto allOf(Entity e) const -> bool
        {
            return allOf<T>(e) && (allOf<Ts>(e) && ...);
        }

        // Get a view for iterating over all components of type T
        template<typename T>
        auto view() -> View<T>
        {
            auto* pool = getPool<T>();
            return View<T>{pool ? pool : getEmptyPoolMutable<std::remove_const_t<T>>()};
        }

        template<typename T>
        auto view() const -> View<std::add_const_t<T>>
        {
            auto const* pool = getPool<T>();
            return View<std::add_const_t<T>>{pool ? pool : getEmptyPool<std::remove_const_t<T>>()};
        }

        template<typename T, typename... Ts>
            requires (sizeof...(Ts) > 0)
        auto view() -> MultiView<T, Ts...>
        {
            return MultiView<T, Ts...>{getPool<T>(), getPool<Ts>()...};
        }

        template<typename T, typename... Ts>
            requires (sizeof...(Ts) > 0)
        auto view() const -> MultiView<std::add_const_t<T>, std::add_const_t<Ts>...>
        {
            return MultiView<std::add_const_t<T>, std::add_const_t<Ts>...>{getPool<T>(), getPool<Ts>()...};
        }

        // Get pool for manual iteration
        template<typename T>
        auto getPool() const -> SparseSet<std::remove_const_t<T>> const*
        {
            auto const typeIndex = std::type_index{typeid(std::remove_const_t<T>)};
            auto it = m_pools.find(typeIndex);
            return it != m_pools.end() ? static_cast<SparseSet<std::remove_const_t<T>> const*>(it->second.get()) : nullptr;
        }

        template<typename T>
        auto getPool() -> SparseSet<std::remove_const_t<T>>*
        {
            return const_cast<SparseSet<std::remove_const_t<T>>*>(static_cast<Registry const*>(this)->getPool<T>());
        }

    private:
        // Get or create a sparse set pool for component type T
        template<typename T>
        auto getOrCreatePool() -> SparseSet<std::remove_const_t<T>>*
        {
            auto* pool = getPool<T>();
            if (pool)
            {
                return pool;
            }

            auto const typeIndex = std::type_index{typeid(std::remove_const_t<T>)};
            auto newPool = std::make_unique<SparseSet<std::remove_const_t<T>>>();
            auto* poolPtr = newPool.get();
            m_pools[typeIndex] = std::move(newPool);
            return poolPtr;
        }

        template<typename T>
        static auto getEmptyPoolMutable() -> SparseSet<T>*
        {
            static SparseSet<T> empty{};
            return &empty;
        }

        template<typename T>
        static auto getEmptyPool() -> SparseSet<T> const*
        {
            return getEmptyPoolMutable<T>();
        }

        std::unordered_map<std::type_index, std::unique_ptr<ISparseSet>> m_pools;
        std::vector<uint32_t> m_generation{};
        std::vector<uint32_t> m_freeIndices{};
    };

} // namespace april::scene
