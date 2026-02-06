#include "editor-camera.hpp"

#include <imgui.h>
#include <glm/glm.hpp>

namespace april::editor
{
    EditorCamera::EditorCamera(float fov, float aspect, float nearClip, float farClip)
    {
        m_fov = fov;
        m_aspectRatio = aspect;
        m_near = nearClip;
        m_far = farClip;
        m_projectionType = EProjectionType::Perspective;

        updateCameraView();
        updateProjectionMatrix();
    }

    auto EditorCamera::onUpdate(float dt) -> void
    {
        auto& io = ImGui::GetIO();
        float2 mouse{io.MousePos.x, io.MousePos.y};

        if (!m_inputEnabled)
        {
            m_initialMousePosition = mouse;
            updateCameraView();
            return;
        }

        if (io.WantCaptureMouse && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            m_initialMousePosition = mouse;
            updateCameraView();
            return;
        }

        float2 delta = (mouse - m_initialMousePosition) * 0.003f;
        m_initialMousePosition = mouse;

        if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            if (delta.x != 0.0f || delta.y != 0.0f)
            {
                float yawSign = getUpDirection().y < 0.0f ? -1.0f : 1.0f;
                m_yaw -= yawSign * delta.x;
                m_pitch += delta.y;
            }

            float speed = 5.0f * dt;
            if (io.KeyShift)
            {
                speed *= 2.5f;
            }
            if (io.KeyCtrl)
            {
                speed *= 0.1f;
            }

            if (ImGui::IsKeyDown(ImGuiKey_W)) m_position += getForwardDirection() * speed;
            if (ImGui::IsKeyDown(ImGuiKey_S)) m_position -= getForwardDirection() * speed;
            if (ImGui::IsKeyDown(ImGuiKey_A)) m_position += getRightDirection() * speed;
            if (ImGui::IsKeyDown(ImGuiKey_D)) m_position -= getRightDirection() * speed;

            if (ImGui::IsKeyDown(ImGuiKey_Q)) m_position -= float3(0.0f, 1.0f, 0.0f) * speed;
            if (ImGui::IsKeyDown(ImGuiKey_E)) m_position += float3(0.0f, 1.0f, 0.0f) * speed;
        }

        updateCameraView();
    }

    auto EditorCamera::setRotation(float pitch, float yaw) -> void
    {
        m_pitch = pitch;
        m_yaw = yaw;
        updateCameraView();
    }

    auto EditorCamera::updateCameraView() -> void
    {
        auto orientation = getRotation();

        m_direction = orientation * float3(0.0f, 0.0f, -1.0f);
        m_up = orientation * float3(0.0f, 1.0f, 0.0f);
        m_right = orientation * float3(1.0f, 0.0f, 0.0f);

        updateViewMatrix();
    }

    auto EditorCamera::getRotation() const -> quaternion
    {
        return quaternion(float3(-m_pitch, -m_yaw, 0.0f));
    }

    auto EditorCamera::getForwardDirection() const -> float3
    {
        return getRotation() * float3(0.0f, 0.0f, -1.0f);
    }

    auto EditorCamera::getRightDirection() const -> float3
    {
        auto const forward = getForwardDirection();
        auto const up = float3(0.0f, 1.0f, 0.0f);
        return glm::normalize(glm::cross(up, forward));
    }

    auto EditorCamera::getUpDirection() const -> float3
    {
        return getRotation() * float3(0.0f, 1.0f, 0.0f);
    }
}
