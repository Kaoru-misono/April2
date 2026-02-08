#include "render-extraction.hpp"

#include <limits>

namespace april::scene
{
    namespace
    {
        auto transformPoint(float4x4 const& matrix, float3 const& point) -> float3
        {
            auto const world = matrix * float4{point, 1.0f};
            return float3{world.x, world.y, world.z};
        }

        auto computeWorldAabb(float4x4 const& matrix, float3 const& localMin, float3 const& localMax) -> AABB
        {
            std::array<float3, 8> corners = {
                float3{localMin.x, localMin.y, localMin.z},
                float3{localMax.x, localMin.y, localMin.z},
                float3{localMin.x, localMax.y, localMin.z},
                float3{localMax.x, localMax.y, localMin.z},
                float3{localMin.x, localMin.y, localMax.z},
                float3{localMax.x, localMin.y, localMax.z},
                float3{localMin.x, localMax.y, localMax.z},
                float3{localMax.x, localMax.y, localMax.z}
            };

            float3 worldMin{std::numeric_limits<float>::max()};
            float3 worldMax{std::numeric_limits<float>::lowest()};

            for (auto const& corner : corners)
            {
                auto const world = transformPoint(matrix, corner);
                worldMin = glm::min(worldMin, world);
                worldMax = glm::max(worldMax, world);
            }

            return AABB{worldMin, worldMax};
        }

        auto findActiveCamera(Registry& registry) -> Entity
        {
            auto const* cameraPool = registry.getPool<CameraComponent>();
            if (!cameraPool || cameraPool->data().empty())
            {
                return NullEntity;
            }

            auto const* tagPool = registry.getPool<TagComponent>();
            if (!tagPool)
            {
                return cameraPool->getEntity(0);
            }

            for (size_t i = 0; i < tagPool->data().size(); ++i)
            {
                auto const entity = tagPool->getEntity(i);
                auto const& tag = tagPool->data()[i];

                if (tag.tag == "MainCamera" && registry.allOf<CameraComponent>(entity))
                {
                    return entity;
                }
            }

            return cameraPool->getEntity(0);
        }

        auto getCameraPosition(Registry& registry, Entity cameraEntity) -> float3
        {
            if (!registry.allOf<TransformComponent>(cameraEntity))
            {
                return float3{0.0f, 0.0f, 0.0f};
            }

            auto const& transform = registry.get<TransformComponent>(cameraEntity);
            auto const translation = transform.worldMatrix[3];
            return float3{translation.x, translation.y, translation.z};
        }
    }

    auto extractFrameSnapshot(Registry& registry, RenderResourceRegistry& resources, FrameSnapshot& snapshot) -> void
    {
        snapshot.reset();

        auto const cameraEntity = findActiveCamera(registry);
        if (cameraEntity != NullEntity && registry.allOf<CameraComponent>(cameraEntity))
        {
            auto const& camera = registry.get<CameraComponent>(cameraEntity);
            snapshot.mainView.viewMatrix = camera.viewMatrix;
            snapshot.mainView.projectionMatrix = camera.projectionMatrix;
            snapshot.mainView.cameraPosition = getCameraPosition(registry, cameraEntity);
            snapshot.mainView.hasCamera = true;
        }

        for (auto [entity, transform, mesh] : registry.view<TransformComponent, MeshRendererComponent>().each())
        {
            static_cast<void>(entity);

            if (!mesh.enabled)
            {
                continue;
            }

            if (mesh.meshId == kInvalidRenderID)
            {
                continue;
            }

            MeshInstance instance{};
            instance.worldTransform = transform.worldMatrix;
            instance.meshId = mesh.meshId;
            instance.materialId = mesh.materialId;

            float3 boundsMin{};
            float3 boundsMax{};
            if (resources.getMeshBounds(mesh.meshId, boundsMin, boundsMax))
            {
                instance.worldBounds = computeWorldAabb(transform.worldMatrix, boundsMin, boundsMax);
            }

            snapshot.dynamicMeshes.push_back(instance);
        }
    }
}
