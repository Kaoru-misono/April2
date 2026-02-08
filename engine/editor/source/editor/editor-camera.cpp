#include "editor-camera.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace april::editor
{
    namespace
    {
        constexpr float kMinPitch = -1.553343f;
        constexpr float kMaxPitch = 1.553343f;
        constexpr float kMinDistance = 0.001f;

        auto safeNormalize(float3 const& value, float3 const& fallback) -> float3
        {
            auto const length = glm::length(value);
            if (length <= 0.000001f)
            {
                return fallback;
            }

            return value / length;
        }
    }

    EditorCamera::EditorCamera(float fov, float aspect, float nearClip, float farClip)
    {
        m_fov = fov;
        m_aspectRatio = aspect;
        m_near = nearClip;
        m_far = farClip;
        m_projectionType = EProjectionType::Perspective;

        m_center = m_position + m_direction * m_distance;

        updateFromLookAt();
        updateProjectionMatrix();
    }

    auto EditorCamera::onUpdate(float dt) -> void
    {
        auto& io = ImGui::GetIO();
        float2 mouse{io.MousePos.x, io.MousePos.y};

        if (!m_inputEnabled)
        {
            m_initialMousePosition = mouse;
            return;
        }

        bool const rmbDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
        bool const mmbDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

        if (io.WantCaptureMouse && !rmbDown && !mmbDown)
        {
            m_initialMousePosition = mouse;
            return;
        }

        if (!rmbDown && !mmbDown)
        {
            m_initialMousePosition = mouse;
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
        {
            m_initialMousePosition = mouse;
        }

        if (rmbDown || mmbDown)
        {
            float2 const displacement = {
                -(mouse.x - m_initialMousePosition.x) / std::max(m_viewportWidth, 1.0f),
                (mouse.y - m_initialMousePosition.y) / std::max(m_viewportHeight, 1.0f),
            };

            m_initialMousePosition = mouse;

            if (rmbDown)
            {
                if (io.KeyCtrl)
                {
                    pan(displacement);
                }
                else if (io.KeyShift)
                {
                    dolly(displacement, false);
                }
                else
                {
                    lookAround(displacement);
                }
            }
            else if (mmbDown)
            {
                pan(displacement);
            }
        }

        if (io.MouseWheel != 0.0f)
        {
            if (io.KeyShift)
            {
                auto fovDegree = glm::degrees(m_fov);
                fovDegree = std::clamp(fovDegree + io.MouseWheel, 1.0f, 179.0f);
                setPerspective(glm::radians(fovDegree), m_near, m_far);
            }
            else
            {
                float const deltaX = (io.MouseWheel * std::abs(io.MouseWheel)) / std::max(m_viewportWidth, 1.0f);
                dolly({deltaX, 0.0f}, io.KeyCtrl);
            }
        }

        if (!io.WantTextInput)
        {
            float speed = m_moveSpeed * dt;
            if (io.KeyShift)
            {
                speed *= 2.5f;
            }
            if (io.KeyCtrl)
            {
                speed *= 0.1f;
            }

            float3 movement{0.0f, 0.0f, 0.0f};
            if (ImGui::IsKeyDown(ImGuiKey_W))
            {
                movement += getForwardDirection();
            }
            if (ImGui::IsKeyDown(ImGuiKey_S))
            {
                movement -= getForwardDirection();
            }
            if (ImGui::IsKeyDown(ImGuiKey_A))
            {
                movement -= getRightDirection();
            }
            if (ImGui::IsKeyDown(ImGuiKey_D))
            {
                movement += getRightDirection();
            }
            if (ImGui::IsKeyDown(ImGuiKey_Q))
            {
                movement -= float3{0.0f, 1.0f, 0.0f};
            }
            if (ImGui::IsKeyDown(ImGuiKey_E))
            {
                movement += float3{0.0f, 1.0f, 0.0f};
            }

            if (glm::length(movement) > 0.0f)
            {
                auto const velocity = safeNormalize(movement, float3{0.0f, 0.0f, 0.0f}) * speed;
                m_position += velocity;
                m_center += velocity;
            }
        }

        updateFromLookAt();
    }

    auto EditorCamera::setViewportSize(uint32_t width, uint32_t height) -> void
    {
        ICamera::setViewportSize(width, height);
    }

    auto EditorCamera::setPosition(float3 const& position) -> void
    {
        m_position = position;
        m_center = m_position + safeNormalize(m_direction, float3{0.0f, 0.0f, -1.0f}) * std::max(m_distance, kMinDistance);
        updateFromLookAt();
    }

    auto EditorCamera::setPerspective(float verticalFov, float nearClip, float farClip) -> void
    {
        ICamera::setPerspective(verticalFov, nearClip, farClip);
    }

    auto EditorCamera::setRotation(float pitch, float yaw) -> void
    {
        m_pitch = std::clamp(pitch, kMinPitch, kMaxPitch);
        m_yaw = yaw;
        updateLookAtFromEuler();
        updateFromLookAt();
    }

    auto EditorCamera::setDistance(float distance) -> void
    {
        m_distance = std::max(distance, kMinDistance);
        m_center = m_position + safeNormalize(m_direction, float3{0.0f, 0.0f, -1.0f}) * m_distance;
        updateFromLookAt();
    }

    auto EditorCamera::updateFromLookAt() -> void
    {
        auto forward = m_center - m_position;
        if (glm::length(forward) <= 0.000001f)
        {
            forward = safeNormalize(m_direction, float3{0.0f, 0.0f, -1.0f});
            m_center = m_position + forward * std::max(m_distance, kMinDistance);
        }

        m_distance = std::max(glm::length(m_center - m_position), kMinDistance);
        m_direction = safeNormalize(m_center - m_position, float3{0.0f, 0.0f, -1.0f});

        float3 worldUp{0.0f, 1.0f, 0.0f};
        m_right = glm::cross(m_direction, worldUp);
        if (glm::length(m_right) <= 0.000001f)
        {
            worldUp = {0.0f, 0.0f, 1.0f};
            m_right = glm::cross(m_direction, worldUp);
        }

        m_right = safeNormalize(m_right, float3{1.0f, 0.0f, 0.0f});
        m_up = safeNormalize(glm::cross(m_right, m_direction), float3{0.0f, 1.0f, 0.0f});

        m_pitch = std::asin(std::clamp(m_direction.y, -0.9999f, 0.9999f));
        m_yaw = std::atan2(m_direction.x, -m_direction.z);

        updateViewMatrix();
    }

    auto EditorCamera::updateLookAtFromEuler() -> void
    {
        m_pitch = std::clamp(m_pitch, kMinPitch, kMaxPitch);

        m_direction.x = std::cos(m_pitch) * std::sin(m_yaw);
        m_direction.y = std::sin(m_pitch);
        m_direction.z = -std::cos(m_pitch) * std::cos(m_yaw);
        m_direction = safeNormalize(m_direction, float3{0.0f, 0.0f, -1.0f});

        m_distance = std::max(m_distance, kMinDistance);
        m_center = m_position + m_direction * m_distance;
    }

    auto EditorCamera::lookAround(float2 displacement) -> void
    {
        if (displacement == float2{0.0f, 0.0f})
        {
            return;
        }

        auto const angularDelta = displacement * glm::two_pi<float>();
        m_yaw -= angularDelta.x;
        m_pitch -= angularDelta.y;
        m_pitch = std::clamp(m_pitch, kMinPitch, kMaxPitch);

        updateLookAtFromEuler();
    }

    auto EditorCamera::orbit(float2 displacement, bool invert) -> void
    {
        if (displacement == float2{0.0f, 0.0f})
        {
            return;
        }

        displacement *= glm::two_pi<float>();

        auto const origin = invert ? m_position : m_center;
        auto const position = invert ? m_center : m_position;

        auto centerToEye = position - origin;
        auto const radius = glm::length(centerToEye);
        if (radius <= 0.000001f)
        {
            return;
        }

        centerToEye = glm::normalize(centerToEye);
        auto const axisZ = centerToEye;

        auto const rotateY = glm::rotate(float4x4{1.0f}, -displacement.x, m_up);
        centerToEye = float3(rotateY * float4{centerToEye, 0.0f});

        auto axisX = glm::cross(m_up, axisZ);
        if (glm::length(axisX) <= 0.000001f)
        {
            return;
        }
        axisX = glm::normalize(axisX);

        auto const rotateX = glm::rotate(float4x4{1.0f}, -displacement.y, axisX);
        auto const rotationVec = float3(rotateX * float4{centerToEye, 0.0f});

        if (glm::sign(rotationVec.x) == glm::sign(centerToEye.x))
        {
            centerToEye = rotationVec;
        }

        centerToEye *= radius;
        auto const newPosition = centerToEye + origin;

        if (invert)
        {
            m_center = newPosition;
        }
        else
        {
            m_position = newPosition;
        }
    }

    auto EditorCamera::pan(float2 displacement) -> void
    {
        auto viewDirection = m_position - m_center;
        auto const viewLength = glm::length(viewDirection);
        if (viewLength <= 0.000001f)
        {
            return;
        }

        auto const viewDistance = viewLength / 0.785f;
        viewDirection = glm::normalize(viewDirection);

        auto rightVector = glm::cross(m_up, viewDirection);
        auto upVector = glm::cross(viewDirection, rightVector);

        rightVector = safeNormalize(rightVector, float3{1.0f, 0.0f, 0.0f});
        upVector = safeNormalize(upVector, float3{0.0f, 1.0f, 0.0f});

        auto const panOffset = (-displacement.x * rightVector + displacement.y * upVector) * viewDistance;
        m_position += panOffset;
        m_center += panOffset;
    }

    auto EditorCamera::dolly(float2 displacement, bool keepCenterFixed) -> void
    {
        auto directionVec = m_center - m_position;
        auto const length = glm::length(directionVec);
        if (length < 0.000001f)
        {
            return;
        }

        float const largerDisplacement = std::abs(displacement.x) > std::abs(displacement.y) ? displacement.x : -displacement.y;
        if (largerDisplacement >= 1.0f)
        {
            return;
        }

        directionVec *= largerDisplacement;
        m_position += directionVec;

        if (!keepCenterFixed)
        {
            m_center += directionVec;
        }
    }

    auto EditorCamera::getForwardDirection() const -> float3
    {
        return m_direction;
    }

    auto EditorCamera::getRightDirection() const -> float3
    {
        return m_right;
    }

    auto EditorCamera::getUpDirection() const -> float3
    {
        return m_up;
    }
}
