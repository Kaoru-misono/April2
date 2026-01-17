// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "sampler.hpp"
#include "parameter-block.hpp"
#include "program/program-reflection.hpp"
#include "program/shader-variable.hpp"

#include <core/math/type.hpp>
#include <core/foundation/object.hpp>

namespace april::graphics
{
    class Device;
    class FullScreenPass; // Assuming exists or will exist

    struct BlitContext
    {
        core::ref<FullScreenPass> pass{};

        core::ref<Sampler> linearSampler{};
        core::ref<Sampler> pointSampler{};
        core::ref<Sampler> linearMinSampler{};
        core::ref<Sampler> pointMinSampler{};
        core::ref<Sampler> linearMaxSampler{};
        core::ref<Sampler> pointMaxSampler{};

        core::ref<ParameterBlock> blitParamsBuffer{};
        float2 prevSrcRectOffset{0, 0};
        float2 prevSrcRectScale{0, 0};

        // Variable offsets in constant buffer
        TypedShaderVariableOffset offsetVariableOffset{};
        TypedShaderVariableOffset scaleVariableOffset{};
        ParameterBlockReflection::BindLocation texBindLocation{};

        // Parameters for complex blit
        float4 prevComponentsTransform[4]{float4(0), float4(0), float4(0), float4(0)};
        TypedShaderVariableOffset compTransVariableOffset[4]{};

        BlitContext(Device* device);
    };
}
