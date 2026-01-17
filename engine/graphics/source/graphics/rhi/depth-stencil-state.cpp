// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "depth-stencil-state.hpp"

#include <core/error/assert.hpp>

namespace april::graphics
{
    auto DepthStencilState::create(Desc const& desc) -> core::ref<DepthStencilState>
    {
        return core::ref<DepthStencilState>(new DepthStencilState(desc));
    }

    DepthStencilState::~DepthStencilState() = default;

    auto DepthStencilState::Desc::setStencilFunc(Face face, ComparisonFunc func) -> Desc&
    {
        if (face == Face::FrontAndBack)
        {
            setStencilFunc(Face::Front, func);
            setStencilFunc(Face::Back, func);
            return *this;
        }
        StencilDesc& desc = (face == Face::Front) ? m_stencilFront : m_stencilBack;
        desc.func = func;
        return *this;
    }

    auto DepthStencilState::Desc::setStencilOp(
        Face face,
        StencilOp stencilFail,
        StencilOp depthFail,
        StencilOp depthStencilPass
    ) -> Desc&
    {
        if (face == Face::FrontAndBack)
        {
            setStencilOp(Face::Front, stencilFail, depthFail, depthStencilPass);
            setStencilOp(Face::Back, stencilFail, depthFail, depthStencilPass);
            return *this;
        }
        StencilDesc& desc = (face == Face::Front) ? m_stencilFront : m_stencilBack;
        desc.stencilFailOp = stencilFail;
        desc.depthFailOp = depthFail;
        desc.depthStencilPassOp = depthStencilPass;

        return *this;
    }

    auto DepthStencilState::getStencilDesc(Face face) const -> StencilDesc const&
    {
        AP_ASSERT(face != Face::FrontAndBack);
        return (face == Face::Front) ? m_desc.m_stencilFront : m_desc.m_stencilBack;
    }
} // namespace april::graphics
