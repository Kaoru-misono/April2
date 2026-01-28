// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "ray-tracing-acceleration-structure-post-build-info-pool.hpp"
#include "command-context.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>

namespace april::graphics
{
    // ...

    auto RtAccelerationStructurePostBuildInfoPool::getElement(CommandContext* context, uint32_t index) -> uint64_t
    {
        return 0;
    }

    auto RtAccelerationStructurePostBuildInfoPool::reset(CommandContext* context) -> void
    {
        // ...
    }
} // namespace april::graphics
