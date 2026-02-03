#pragma once

#include "camera.hpp"
#include <core/window/window.hpp>

namespace april
{
    struct SimpleCamera: public ICamera
    {
        SimpleCamera(float fov, float aspect, float nearClip, float farClip);

        auto onUpdate(float dt) -> void override;
        auto setInputEnabled(bool enabled) -> void { m_inputEnabled = enabled; }

        [[nodiscard]] auto getDistance() const -> float { return m_distance; }
        auto setDistance(float distance) -> void { m_distance = distance; }

    private:
        auto updateCameraView() -> void;

        auto getRotation() const -> quaternion;
        auto getForwardDirection() const -> float3;
        auto getRightDirection() const -> float3;
        auto getUpDirection() const -> float3;

    private:
        float m_distance = 10.0f;
        float2 m_initialMousePosition = { 0.0f, 0.0f };
        bool m_inputEnabled = true;

        float m_pitch = 0.0f;
        float m_yaw = 0.0f;
    };
}
