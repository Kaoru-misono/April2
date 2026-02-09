#pragma once

#include <graphics/camera/camera.hpp>

namespace april::editor
{
    class EditorCamera final : public ICamera
    {
    public:
        EditorCamera(float fov, float aspect, float nearClip, float farClip);

        auto onUpdate(float dt) -> void override;
        auto setViewportSize(uint32_t width, uint32_t height) -> void override;
        auto setInputEnabled(bool enabled) -> void { m_inputEnabled = enabled; }

        auto setPosition(float3 const& position) -> void;
        auto setPerspective(float verticalFov, float nearClip, float farClip) -> void;
        auto setRotation(float pitch, float yaw) -> void;
        [[nodiscard]] auto getDistance() const -> float { return m_distance; }
        auto setDistance(float distance) -> void;

    private:
        auto updateFromLookAt() -> void;
        auto updateLookAtFromEuler() -> void;
        auto lookAround(float2 displacement) -> void;

        auto orbit(float2 displacement, bool invert) -> void;
        auto pan(float2 displacement) -> void;
        auto dolly(float2 displacement, bool keepCenterFixed) -> void;

        [[nodiscard]] auto getForwardDirection() const -> float3;
        [[nodiscard]] auto getRightDirection() const -> float3;
        [[nodiscard]] auto getUpDirection() const -> float3;

    private:
        float3 m_center = {0.0f, 0.0f, 0.0f};
        float m_distance = 10.0f;
        float2 m_initialMousePosition = {0.0f, 0.0f};
        bool m_inputEnabled = true;
        float m_pitch = 0.0f;
        float m_yaw = 0.0f;
        float m_moveSpeed = 5.0f;
    };
}
