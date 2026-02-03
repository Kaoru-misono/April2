// Example usage of the Scene ECS system
// This file is not compiled - it's for documentation purposes

#include "scene/scene.hpp"

using namespace april::scene;

auto exampleUsage() -> void
{
    // Create a scene graph
    SceneGraph scene{};

    // Create entities with names
    auto root = scene.createEntity("Root");
    auto child1 = scene.createEntity("Child1");
    auto child2 = scene.createEntity("Child2");

    // Access registry directly for component operations
    auto& registry = scene.getRegistry();

    // Get components (EnTT-style API)
    auto& transform = registry.get<TransformComponent>(root);
    transform.localPosition = float3{0.f, 1.f, 0.f};
    transform.localRotation = float3{0.f, glm::radians(45.f), 0.f};
    transform.localScale = float3{2.f, 2.f, 2.f};

    auto& tag = registry.get<TagComponent>(child1);
    tag.tag = "Updated Name";

    // Check if entity has component
    if (registry.all_of<TransformComponent>(child1))
    {
        // Do something...
    }

    // Set up hierarchy
    scene.setParent(child1, root);
    scene.setParent(child2, root);

    // Iterate over all components of a type (EnTT-style view)
    for (auto& transform : registry.view<TransformComponent>())
    {
        transform.isDirty = true;
    }

    // Update transforms (will recursively update hierarchy)
    auto const identityMatrix = glm::mat4{1.f};
    // Note: updateTransform is private, so you'd call it through onUpdateRuntime
    // or make it public if needed

    // Add custom components
    struct VelocityComponent
    {
        float3 velocity{0.f};
    };

    registry.emplace<VelocityComponent>(child1, float3{1.f, 0.f, 0.f});

    // Iterate custom components
    for (auto& velocity : registry.view<VelocityComponent>())
    {
        // Update velocity...
    }

    // Destroy entity (will also destroy all children)
    scene.destroyEntity(root);
}

// Example: Migration from EnTT is straightforward
// Just replace entt::registry with april::scene::Registry
//
// EnTT code:
//   entt::registry registry;
//   auto entity = registry.create();
//   registry.emplace<Transform>(entity, position, rotation);
//   auto& transform = registry.get<Transform>(entity);
//   for (auto& comp : registry.view<Transform>()) { ... }
//
// April Scene code:
//   april::scene::Registry registry;
//   auto entity = registry.create();
//   registry.emplace<Transform>(entity, position, rotation);
//   auto& transform = registry.get<Transform>(entity);
//   for (auto& comp : registry.view<Transform>()) { ... }
