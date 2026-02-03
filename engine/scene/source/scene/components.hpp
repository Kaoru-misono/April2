#pragma once

#include "ecs-core.hpp"
#include "core/tools/uuid.hpp"
#include "core/math/type.hpp"
#include <string>

namespace april::scene
{
    // Unique identifier for each entity
    struct IDComponent
    {
        core::UUID id{};
    };

    // Human-readable tag/name for entities
    struct TagComponent
    {
        std::string tag{};
    };

    // Transform component with local and world space data
    struct TransformComponent
    {
        // Local transform
        float3 localPosition{0.f, 0.f, 0.f};
        float3 localRotation{0.f, 0.f, 0.f}; // Euler angles in radians
        float3 localScale{1.f, 1.f, 1.f};

        // World transform (computed from hierarchy)
        float4x4 worldMatrix{1.f};

        // Dirty flag for transform updates
        bool isDirty = true;

        // Helper to get local transform matrix
        [[nodiscard]] auto getLocalMatrix() const -> float4x4
        {
            auto matrix = glm::mat4{1.f};

            // Translation
            matrix = glm::translate(matrix, localPosition);

            // Rotation (Euler XYZ)
            matrix = glm::rotate(matrix, localRotation.x, float3{1.f, 0.f, 0.f});
            matrix = glm::rotate(matrix, localRotation.y, float3{0.f, 1.f, 0.f});
            matrix = glm::rotate(matrix, localRotation.z, float3{0.f, 0.f, 1.f});

            // Scale
            matrix = glm::scale(matrix, localScale);

            return matrix;
        }
    };

    // Hierarchy relationship component using linked list structure
    struct RelationshipComponent
    {
        size_t childrenCount = 0;

        Entity parent = NullEntity;
        Entity firstChild = NullEntity;
        Entity prevSibling = NullEntity;
        Entity nextSibling = NullEntity;
    };

    // Mesh renderer component for entity-based rendering
    struct MeshRendererComponent
    {
        std::string meshAssetPath{};      // Path to .asset file
        uint32_t materialId{0};           // Material index (0 = default)
        bool castShadows{true};           // Future shadow system
        bool receiveShadows{true};
        bool enabled{true};               // Toggle rendering
    };

    // Camera component for view/projection rendering
    struct CameraComponent
    {
        // Projection
        bool isPerspective{true};
        float fov{glm::radians(45.0f)};
        float orthoSize{10.0f};
        float nearClip{0.1f};
        float farClip{1000.0f};

        // Viewport
        uint32_t viewportWidth{1920};
        uint32_t viewportHeight{1080};

        // Computed matrices
        float4x4 viewMatrix{1.0f};
        float4x4 projectionMatrix{1.0f};
        float4x4 viewProjectionMatrix{1.0f};

        bool isDirty{true};
    };

} // namespace april::scene
