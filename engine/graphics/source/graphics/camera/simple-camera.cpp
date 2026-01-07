#include "simple-camera.hpp"
#include <imgui.h>

namespace april
{
    SimpleCamera::SimpleCamera(float fov, float aspect, float nearClip, float farClip)
    {
        m_fov = fov;
        m_aspectRatio = aspect;
        m_near = nearClip;
        m_far = farClip;
        m_projectionType = EProjectionType::Perspective;

        updateCameraView();
        updateProjectionMatrix();
    }

    auto SimpleCamera::onUpdate(float dt) -> void
    {
        auto& io = ImGui::GetIO();

        if (io.WantCaptureMouse) return;

        const float2 mouse{ io.MousePos.x, io.MousePos.y };
        float2 delta = (mouse - m_initialMousePosition) * 0.003f;
        m_initialMousePosition = mouse;

        if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            if (delta.x != 0.0f || delta.y != 0.0f)
            {
                float yawSign = getUpDirection().y < 0 ? -1.0f : 1.0f;
                m_yaw   += yawSign * delta.x;
                m_pitch += delta.y;
            }

            float speed = 5.0f * dt;

            if (io.KeyShift) speed *= 2.5f;
            if (io.KeyCtrl) speed *= 0.1f;

            if (ImGui::IsKeyDown(ImGuiKey_W)) m_position += getForwardDirection() * speed;
            if (ImGui::IsKeyDown(ImGuiKey_S)) m_position -= getForwardDirection() * speed;
            if (ImGui::IsKeyDown(ImGuiKey_A)) m_position -= getRightDirection() * speed;
            if (ImGui::IsKeyDown(ImGuiKey_D)) m_position += getRightDirection() * speed;

            if (ImGui::IsKeyDown(ImGuiKey_Q)) m_position -= float3(0.0f, 1.0f, 0.0f) * speed;
            if (ImGui::IsKeyDown(ImGuiKey_E)) m_position += float3(0.0f, 1.0f, 0.0f) * speed;
        }

        updateCameraView();
    }

    auto SimpleCamera::updateCameraView() -> void
    {
        auto orientation = getRotation();

        m_direction = orientation * float3(0.0f, 0.0f, -1.0f);
        m_up        = orientation * float3(0.0f, 1.0f, 0.0f);
        m_right     = orientation * float3(1.0f, 0.0f, 0.0f);

        updateViewMatrix();
    }

    auto SimpleCamera::getRotation() const -> quaternion
    {
        return quaternion(float3(-m_pitch, -m_yaw, 0.0f));
    }

    auto SimpleCamera::getForwardDirection() const -> float3
    {
        return getRotation() * float3(0.0f, 0.0f, -1.0f);
    }

    auto SimpleCamera::getRightDirection() const -> float3
    {
        return getRotation() * float3(1.0f, 0.0f, 0.0f);
    }

    auto SimpleCamera::getUpDirection() const -> float3
    {
        return getRotation() * float3(0.0f, 1.0f, 0.0f);
    }
}
