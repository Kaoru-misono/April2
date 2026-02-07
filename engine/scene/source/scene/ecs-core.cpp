#include "ecs-core.hpp"

namespace april::scene
{
    auto operator==(Entity lhs, Entity rhs) -> bool
    {
        return lhs.index == rhs.index && lhs.generation == rhs.generation;
    }

    auto operator!=(Entity lhs, Entity rhs) -> bool
    {
        return !(lhs == rhs);
    }

    auto isNull(Entity e) -> bool
    {
        return e.index == kInvalidEntityIndex;
    }

    auto toIntegral(Entity e) -> uint64_t
    {
        return (static_cast<uint64_t>(e.generation) << 32u) | static_cast<uint64_t>(e.index);
    }

    auto EntityHash::operator()(Entity e) const noexcept -> size_t
    {
        return static_cast<size_t>(toIntegral(e));
    }

    auto Registry::create() -> Entity
    {
        uint32_t index = 0;
        if (!m_freeIndices.empty())
        {
            index = m_freeIndices.back();
            m_freeIndices.pop_back();
        }
        else
        {
            index = static_cast<uint32_t>(m_generation.size());
            m_generation.push_back(0);
        }

        return Entity{index, m_generation[index]};
    }

    auto Registry::destroy(Entity e) -> void
    {
        if (!valid(e))
        {
            return;
        }

        for (auto& [type, pool] : m_pools)
        {
            pool->remove(e);
        }

        ++m_generation[e.index];
        m_freeIndices.push_back(e.index);
    }

    auto Registry::valid(Entity e) const -> bool
    {
        return !isNull(e)
            && e.index < m_generation.size()
            && m_generation[e.index] == e.generation;
    }
}
