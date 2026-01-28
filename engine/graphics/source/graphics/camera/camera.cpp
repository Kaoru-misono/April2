#include "camera.hpp"

namespace april
{
    auto ICamera::setViewportSize(uint32_t width, uint32_t height) -> void
    {
        if (height == 0) height = 1;

        m_viewportWidth = static_cast<float>(width);
        m_viewportHeight = static_cast<float>(height);
        m_aspectRatio = m_viewportWidth / m_viewportHeight;

        updateProjectionMatrix();
    }

    auto ICamera::setPerspective(float verticalFov, float nearClip, float farClip) -> void
    {
        m_projectionType = EProjectionType::Perspective;
        m_fov = verticalFov;
        m_near = nearClip;
        m_far = farClip;

        updateProjectionMatrix();
    }

    auto ICamera::updateViewMatrix() -> void
    {
        m_viewMatrix = math::lookAt(m_position, m_position + m_direction, m_up);

        m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
    }

    auto ICamera::updateProjectionMatrix() -> void
    {
        if (m_projectionType == EProjectionType::Perspective)
        {
            m_projectionMatrix = math::perspective(m_fov, m_aspectRatio, m_near, m_far);

            if (m_flipY) {
                m_projectionMatrix[1][1] *= -1.0f;
            }
        }
        else
        {
            // float orthoLeft = -m_viewportWidth * 0.5f;
            // float orthoRight = m_viewportWidth * 0.5f;
            // float orthoBottom = -m_viewportHeight * 0.5f;
            // float orthoTop = m_viewportHeight * 0.5f;
            // m_projectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_near, m_far);
        }

        m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
    }
}
