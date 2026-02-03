#pragma once

#include "camera.hpp"

namespace april
{
    struct FixedCamera final : public ICamera
    {
        FixedCamera(
            float3 const& position,
            float3 const& target,
            float3 const& up,
            float verticalFov,
            float aspect,
            float nearClip,
            float farClip
        );

        auto setLookAt(float3 const& position, float3 const& target, float3 const& up) -> void;
    };
}
