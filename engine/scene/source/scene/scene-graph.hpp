#pragma once

#include "ecs-core.hpp"

#include <core/math/type.hpp>

#include <string>
#include <vector>

namespace april::scene
{
    // Scene graph manager with entity lifecycle and hierarchy management
    class SceneGraph
    {
    public:
        SceneGraph() = default;

        // Entity lifecycle
        auto createEntity(std::string const& name) -> Entity;
        auto destroyEntity(Entity e) -> void;

        // Hierarchy management
        auto setParent(Entity child, Entity newParent) -> void;

        // Transform system
        auto onUpdateRuntime(float dt) -> void;
        auto updateTransforms() -> void;
        auto updateCameras(uint32_t viewportWidth, uint32_t viewportHeight) -> void;
        auto markTransformDirty(Entity e) -> void;

        // Camera system
        auto getActiveCamera() const -> Entity;

        // Registry access
        auto getRegistry() -> Registry&;
        auto getRegistry() const -> Registry const&;

    private:
        // Transform update helpers
        auto updateTransform(Entity e, float4x4 const& parentMatrix, bool parentDirty) -> void;

        // Hierarchy helpers
        auto unlinkFromParent(Entity child) -> void;
        auto linkToParent(Entity child, Entity parent) -> void;
        auto destroyEntityRecursive(Entity e) -> void;
        auto isDescendant(Entity maybeDescendant, Entity ancestor) const -> bool;
        auto addRoot(Entity e) -> void;
        auto removeRoot(Entity e) -> void;
        auto addDirtyRoot(Entity e) -> void;
        auto removeDirtyRoot(Entity e) -> void;
        auto isCoveredByDirtyRoots(Entity e) const -> bool;

        Registry m_registry;
        std::vector<Entity> m_roots{};
        std::vector<Entity> m_dirtyRoots{};
    };

} // namespace april::scene
