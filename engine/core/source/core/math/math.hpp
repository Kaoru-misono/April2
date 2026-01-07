#pragma once

#include "type.hpp"

namespace april
{
    inline auto lerp(float from, float to, float t) -> float
    {
        return from + t * (to - from);
    }

    // http://www.rorydriscoll.com/2016/03/07/frame-rate-independent-damping-using-lerp/
    inline auto lerp(float from, float to, float t, float speed) -> float
    {
        return lerp(from, to, 1.0f - std::pow(1.0f - t, speed));
    }

    inline auto lerpFloat3(float3 from, float3 to, float t, float speed) -> float3
    {
        return {lerp(from.x, to.x, t, speed), lerp(from.y, to.y, t, speed), lerp(from.z, to.z, t, speed)};
    }

    template <typename T>
    constexpr auto saturate(T const& v) -> T
    {
        return glm::clamp(v, T(0.0f), T(1.0f));
    }

    inline auto rotationFromAngleAxis(float angle_degree, float3 axis) -> float4x4
    {
        auto mat = float4x4{1.0f};
        mat = glm::rotate(mat, glm::radians(angle_degree), axis);
        return mat;
    }

    inline auto translationFromPosition(float3 position) -> float4x4
    {
        auto mat = float4x4{1.0f};
        mat = glm::translate(mat, position);
        return mat;
    }

    template <typename T>
    inline auto inverseTranspose(T const& mat) -> T
    {
        return glm::transpose(glm::inverse(mat));
    }

    template <typename T>
    inline auto divideRoundingUp(T const& a, T const& b) -> T
    {
        return (a + b - (T) 1) / b;
    }
} // namespace april
