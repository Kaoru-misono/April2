#pragma once

#include "ecs-core.hpp"
#include "components.hpp"
#include <string>

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

        // Camera system
        auto getActiveCamera() const -> Entity;

        // Registry access
        auto getRegistry() -> Registry& { return m_registry; }
        auto getRegistry() const -> Registry const& { return m_registry; }

    private:
        // Transform update helpers
        auto updateTransform(Entity e, float4x4 const& parentMatrix, bool parentDirty) -> void;

        // Hierarchy helpers
        auto unlinkFromParent(Entity child) -> void;
        auto linkToParent(Entity child, Entity parent) -> void;
        auto destroyEntityRecursive(Entity e) -> void;

        Registry m_registry;
    };

} // namespace april::scene
