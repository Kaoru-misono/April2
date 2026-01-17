// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "blend-state.hpp"

#include <core/error/assert.hpp>

namespace april::graphics
{
    auto BlendState::create(Desc const& desc) -> core::ref<BlendState>
    {
        return core::ref<BlendState>(new BlendState(desc));
    }

    BlendState::Desc::Desc()
    {
        m_rtDesc.resize(8);
    }

    BlendState::~BlendState() = default;

    auto BlendState::Desc::setRtParams(
        uint32_t rtIndex,
        BlendOp rgbOp,
        BlendOp alphaOp,
        BlendFunc srcRgbFunc,
        BlendFunc dstRgbFunc,
        BlendFunc srcAlphaFunc,
        BlendFunc dstAlphaFunc
    ) -> Desc&
    {
        AP_ASSERT(rtIndex < m_rtDesc.size(), "'rtIndex' ({}) is out of range. Must be smaller than {}.", rtIndex, m_rtDesc.size());

        m_rtDesc[rtIndex].rgbBlendOp = rgbOp;
        m_rtDesc[rtIndex].alphaBlendOp = alphaOp;
        m_rtDesc[rtIndex].srcRgbFunc = srcRgbFunc;
        m_rtDesc[rtIndex].dstRgbFunc = dstRgbFunc;
        m_rtDesc[rtIndex].srcAlphaFunc = srcAlphaFunc;
        m_rtDesc[rtIndex].dstAlphaFunc = dstAlphaFunc;
        return *this;
    }

    auto BlendState::Desc::setRenderTargetWriteMask(
        uint32_t rtIndex,
        bool writeRed,
        bool writeGreen,
        bool writeBlue,
        bool writeAlpha
    ) -> Desc&
    {
        AP_ASSERT(rtIndex < m_rtDesc.size(), "'rtIndex' ({}) is out of range. Must be smaller than {}.", rtIndex, m_rtDesc.size());

        m_rtDesc[rtIndex].writeMask.writeRed = writeRed;
        m_rtDesc[rtIndex].writeMask.writeGreen = writeGreen;
        m_rtDesc[rtIndex].writeMask.writeBlue = writeBlue;
        m_rtDesc[rtIndex].writeMask.writeAlpha = writeAlpha;
        return *this;
    }
} // namespace april::graphics
