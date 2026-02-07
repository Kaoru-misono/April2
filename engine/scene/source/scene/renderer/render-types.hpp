#pragma once

#include <core/math/type.hpp>

#include <vector>
#include <cstdint>

namespace april::scene
{
    using RenderID = uint32_t;
    inline constexpr RenderID kInvalidRenderID = 0;

    struct AABB
    {
        float3 min{0.0f, 0.0f, 0.0f};
        float3 max{0.0f, 0.0f, 0.0f};
    };

    struct MeshInstance
    {
        float4x4 worldTransform{1.0f};
        RenderID meshId{kInvalidRenderID};
        RenderID materialId{kInvalidRenderID};
        AABB worldBounds{};
    };

    struct LightInstance
    {
        float3 position{0.0f, 0.0f, 0.0f};
        float3 color{1.0f, 1.0f, 1.0f};
        float radius{1.0f};
        float intensity{1.0f};
    };

    struct ViewSnapshot
    {
        float4x4 viewMatrix{1.0f};
        float4x4 projectionMatrix{1.0f};
        float3 cameraPosition{0.0f, 0.0f, 0.0f};
        bool hasCamera{false};
    };

    struct FrameSnapshot
    {
        ViewSnapshot mainView{};
        std::vector<MeshInstance> staticMeshes{};
        std::vector<MeshInstance> dynamicMeshes{};
        std::vector<LightInstance> lights{};

        auto reset() -> void;
    };
}
