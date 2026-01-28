#pragma once

#include <core/math/type.hpp>

namespace april
{
    enum struct EProjectionType
    {
        Perspective,
        Orthographic
    };

    struct ICamera
    {
        ICamera() = default;
        virtual ~ICamera() = default;

        [[nodiscard]] auto getViewMatrix() const -> float4x4 const& { return m_viewMatrix; }
        [[nodiscard]] auto getProjectionMatrix() const -> float4x4 const& { return m_projectionMatrix; }
        [[nodiscard]] auto getViewProjectionMatrix() const -> float4x4 const& { return m_viewProjectionMatrix; }

        [[nodiscard]] auto getPosition() const -> float3 const& { return m_position; }
        [[nodiscard]] auto getDirection() const -> float3 const& { return m_direction; }
        [[nodiscard]] auto getRight() const -> float3 const& { return m_right; }
        [[nodiscard]] auto getUp() const -> float3 const& { return m_up; }

        virtual auto setViewportSize(uint32_t width, uint32_t height) -> void;

        auto setPosition(float3 const& position) -> void { m_position = position; updateViewMatrix(); }

        auto setPerspective(float verticalFov, float nearClip, float farClip) -> void;

        virtual auto onUpdate(float dt) -> void { (void)dt; }

    protected:
        auto updateViewMatrix() -> void;
        auto updateProjectionMatrix() -> void;

    protected:
        EProjectionType m_projectionType = EProjectionType::Perspective;
        bool m_flipY = false;

        // Config
        float m_viewportWidth = 1280.0f;
        float m_viewportHeight = 720.0f;
        float m_aspectRatio = 1.777f;

        // Perspective Config
        float m_fov = glm::radians(45.0f);
        float m_near = 0.1f;
        float m_far = 1000.0f;

        // Transform
        float3 m_position = {0.0f, 0.0f, 0.0f};
        float3 m_direction = {0.0f, 0.0f, -1.0f};
        float3 m_up = {0.0f, 1.0f, 0.0f};
        float3 m_right = {1.0f, 0.0f, 0.0f};

        // Matrices
        float4x4 m_viewMatrix{1.0f};
        float4x4 m_projectionMatrix{1.0f};
        float4x4 m_viewProjectionMatrix{1.0f};
    };
}
