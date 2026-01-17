// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include <core/math/type.hpp>
#include <core/foundation/object.hpp>
#include <vector>

namespace april::graphics
{
    class BlendState : public core::Object
    {
        APRIL_OBJECT(BlendState)
    public:
        enum class BlendOp
        {
            Add,
            Subtract,        //< Subtract src1 from src2
            ReverseSubtract, //< Subtract src2 from src1
            Min,
            Max
        };

        enum class BlendFunc
        {
            Zero,                ///< (0, 0, 0, 0)
            One,                 ///< (1, 1, 1, 1)
            SrcColor,            ///< The fragment-shader output color
            OneMinusSrcColor,    ///< One minus the fragment-shader output color
            DstColor,            ///< The render-target color
            OneMinusDstColor,    ///< One minus the render-target color
            SrcAlpha,            ///< The fragment-shader output alpha value
            OneMinusSrcAlpha,    ///< One minus the fragment-shader output alpha value
            DstAlpha,            ///< The render-target alpha value
            OneMinusDstAlpha,    ///< One minus the render-target alpha value
            BlendFactor,         ///< Constant color, set using Desc#SetBlendFactor()
            OneMinusBlendFactor, ///< One minus constant color, set using Desc#SetBlendFactor()
            SrcAlphaSaturate,    ///< (f, f, f, 1), where f = min(fragment shader output alpha, 1 - render-target pixel alpha)
            Src1Color,           ///< Fragment-shader output color 1
            OneMinusSrc1Color,   ///< One minus fragment-shader output color 1
            Src1Alpha,           ///< Fragment-shader output alpha 1
            OneMinusSrc1Alpha    ///< One minus fragment-shader output alpha 1
        };

        class Desc
        {
        public:
            Desc();
            friend class BlendState;

            auto setBlendFactor(float4 const& factor) -> Desc&
            {
                m_blendFactor = factor;
                return *this;
            }

            auto setIndependentBlend(bool enabled) -> Desc&
            {
                m_enableIndependentBlend = enabled;
                return *this;
            }

            auto setRtParams(
                uint32_t rtIndex,
                BlendOp rgbOp,
                BlendOp alphaOp,
                BlendFunc srcRgbFunc,
                BlendFunc dstRgbFunc,
                BlendFunc srcAlphaFunc,
                BlendFunc dstAlphaFunc
            ) -> Desc&;

            auto setRtBlend(uint32_t rtIndex, bool enable) -> Desc&
            {
                if (rtIndex >= m_rtDesc.size()) m_rtDesc.resize(rtIndex + 1);
                m_rtDesc[rtIndex].blendEnabled = enable;
                return *this;
            }

            auto setAlphaToCoverage(bool enabled) -> Desc&
            {
                m_alphaToCoverageEnabled = enabled;
                return *this;
            }

            auto setRenderTargetWriteMask(uint32_t rtIndex, bool writeRed, bool writeGreen, bool writeBlue, bool writeAlpha) -> Desc&;

            struct RenderTargetDesc
            {
                bool blendEnabled{false};
                BlendOp rgbBlendOp{BlendOp::Add};
                BlendOp alphaBlendOp{BlendOp::Add};
                BlendFunc srcRgbFunc{BlendFunc::One};
                BlendFunc srcAlphaFunc{BlendFunc::One};
                BlendFunc dstRgbFunc{BlendFunc::Zero};
                BlendFunc dstAlphaFunc{BlendFunc::Zero};
                struct WriteMask
                {
                    bool writeRed{true};
                    bool writeGreen{true};
                    bool writeBlue{true};
                    bool writeAlpha{true};
                };
                WriteMask writeMask;
            };

            std::vector<RenderTargetDesc> m_rtDesc;
            bool m_enableIndependentBlend{false};
            bool m_alphaToCoverageEnabled{false};
            float4 m_blendFactor{0, 0, 0, 0};
        };

        virtual ~BlendState();

        static auto create(Desc const& desc) -> core::ref<BlendState>;

        auto getBlendFactor() const -> float4 const& { return m_desc.m_blendFactor; }

        auto getRgbBlendOp(uint32_t rtIndex) const -> BlendOp { return m_desc.m_rtDesc[rtIndex].rgbBlendOp; }
        auto getAlphaBlendOp(uint32_t rtIndex) const -> BlendOp { return m_desc.m_rtDesc[rtIndex].alphaBlendOp; }
        auto getSrcRgbFunc(uint32_t rtIndex) const -> BlendFunc { return m_desc.m_rtDesc[rtIndex].srcRgbFunc; }
        auto getSrcAlphaFunc(uint32_t rtIndex) const -> BlendFunc { return m_desc.m_rtDesc[rtIndex].srcAlphaFunc; }
        auto getDstRgbFunc(uint32_t rtIndex) const -> BlendFunc { return m_desc.m_rtDesc[rtIndex].dstRgbFunc; }
        auto getDstAlphaFunc(uint32_t rtIndex) const -> BlendFunc { return m_desc.m_rtDesc[rtIndex].dstAlphaFunc; }

        auto isBlendEnabled(uint32_t rtIndex) const -> bool { return m_desc.m_rtDesc[rtIndex].blendEnabled; }
        auto isAlphaToCoverageEnabled() const -> bool { return m_desc.m_alphaToCoverageEnabled; }
        auto isIndependentBlendEnabled() const -> bool { return m_desc.m_enableIndependentBlend; }

        auto getRtDesc(size_t rtIndex) const -> Desc::RenderTargetDesc const& { return m_desc.m_rtDesc[rtIndex]; }
        auto getRtCount() const -> uint32_t { return (uint32_t)m_desc.m_rtDesc.size(); }

    private:
        BlendState(Desc const& desc) : m_desc(desc) {}
        const Desc m_desc;
    };
}