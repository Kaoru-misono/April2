// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include <core/tools/enum.hpp>
#include <core/tools/enum-flags.hpp>
#include <core/math/type.hpp>
#include <cstdint>

namespace april::graphics
{
    /**
     * Raytracing pipeline flags.
     */
    enum class RtPipelineFlags : uint32_t
    {
        None = 0,
        SkipTriangles = (1 << 0),
        SkipProcedurals = (1 << 1),
        EnableSpheres = (1 << 2),
        EnableLinearSweptSpheres = (1 << 3),
        EnableClusters = (1 << 4),
        EnableMotion = (1 << 5)
    };
    AP_ENUM_CLASS_OPERATORS(RtPipelineFlags);

    /**
     * Raytracing axis-aligned bounding box.
     */
    struct RtAABB
    {
        float3 min{};
        float3 max{};
    };

    /**
     * Flags passed to TraceRay(). These must match the device side.
     */
    enum class RayFlags : uint32_t
    {
        None = 0, // Explicit 0
        ForceOpaque = 0x1,
        ForceNonOpaque = 0x2,
        AcceptFirstHitAndEndSearch = 0x4,
        SkipClosestHitShader = 0x8,
        CullBackFacingTriangles = 0x10,
        CullFrontFacingTriangles = 0x20,
        CullOpaque = 0x40,
        CullNonOpaque = 0x80,
        SkipTriangles = 0x100,
        SkipProceduralPrimitives = 0x200,
    };
    AP_ENUM_CLASS_OPERATORS(RayFlags);

    // Maximum raytracing attribute size.
    constexpr auto getRaytracingMaxAttributeSize() -> uint32_t
    {
        return 32;
    }
}
