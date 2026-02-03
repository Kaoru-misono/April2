#include "fixed-camera.hpp"

namespace april
{
    FixedCamera::FixedCamera(
        float3 const& position,
        float3 const& target,
        float3 const& up,
        float verticalFov,
        float aspect,
        float nearClip,
        float farClip
    )
    {
        m_projectionType = EProjectionType::Perspective;
        m_fov = verticalFov;
        m_aspectRatio = aspect;
        m_near = nearClip;
        m_far = farClip;

        setLookAt(position, target, up);
        updateProjectionMatrix();
    }

    auto FixedCamera::setLookAt(float3 const& position, float3 const& target, float3 const& up) -> void
    {
        m_position = position;
        auto direction = target - position;
        if (math::length(direction) <= 0.00001f)
        {
            direction = float3{0.0f, 0.0f, -1.0f};
        }

        m_direction = math::normalize(direction);
        m_right = math::normalize(math::cross(m_direction, up));
        m_up = math::normalize(math::cross(m_right, m_direction));

        updateViewMatrix();
    }
}
