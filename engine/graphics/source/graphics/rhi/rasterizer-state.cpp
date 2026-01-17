// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "rasterizer-state.hpp"

namespace april::graphics
{
    auto RasterizerState::create(Desc const& desc) -> core::ref<RasterizerState>
    {
        return core::ref<RasterizerState>(new RasterizerState(desc));
    }

    RasterizerState::~RasterizerState() = default;
} // namespace april::graphics
