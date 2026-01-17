// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include <cstdint>

namespace april::graphics
{
    struct DispatchArguments
    {
        uint32_t ThreadGroupCountX{0};
        uint32_t ThreadGroupCountY{0};
        uint32_t ThreadGroupCountZ{0};
    };

    struct DrawArguments
    {
        uint32_t VertexCountPerInstance{0};
        uint32_t InstanceCount{0};
        uint32_t StartVertexLocation{0};
        uint32_t StartInstanceLocation{0};
    };

    struct DrawIndexedArguments
    {
        uint32_t IndexCountPerInstance{0};
        uint32_t InstanceCount{0};
        uint32_t StartIndexLocation{0};
        int32_t BaseVertexLocation{0};
        uint32_t StartInstanceLocation{0};
    };
}