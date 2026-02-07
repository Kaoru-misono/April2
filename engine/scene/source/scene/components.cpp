#include "components.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace april::scene
{
    auto TransformComponent::getLocalMatrix() const -> float4x4
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
}
