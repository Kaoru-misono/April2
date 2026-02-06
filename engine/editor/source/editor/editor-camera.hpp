#pragma once

#include <graphics/camera/camera.hpp>

namespace april::editor
{
    class EditorCamera final : public ICamera
    {
    public:
        EditorCamera(float fov, float aspect, float nearClip, float farClip);

        auto onUpdate(float dt) -> void override;
        auto setInputEnabled(bool enabled) -> void { m_inputEnabled = enabled; }
        auto setRotation(float pitch, float yaw) -> void;

        [[nodiscard]] auto getDistance() const -> float { return m_distance; }
        auto setDistance(float distance) -> void { m_distance = distance; }

    private:
        auto updateCameraView() -> void;

        [[nodiscard]] auto getRotation() const -> quaternion;
        [[nodiscard]] auto getForwardDirection() const -> float3;
        [[nodiscard]] auto getRightDirection() const -> float3;
        [[nodiscard]] auto getUpDirection() const -> float3;

    private:
        float m_distance = 10.0f;
        float2 m_initialMousePosition = {0.0f, 0.0f};
        bool m_inputEnabled = true;

        float m_pitch = 0.0f;
        float m_yaw = 0.0f;
    };
}
