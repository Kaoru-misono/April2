// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include <core/foundation/object.hpp>
#include <core/tools/enum.hpp>
#include <cstdint>

namespace april::graphics
{
    class RasterizerState : public core::Object
    {
        APRIL_OBJECT(RasterizerState)
    public:
        enum class CullMode : uint32_t
        {
            None,
            Front,
            Back,
        };
        AP_ENUM_INFO(
            CullMode,
            {
                {CullMode::None, "None"},
                {CullMode::Front, "Front"},
                {CullMode::Back, "Back"},
            }
        );

        enum class FillMode
        {
            Wireframe,
            Solid
        };

        class Desc
        {
        public:
            friend class RasterizerState;

            auto setCullMode(CullMode mode) -> Desc&
            {
                m_cullMode = mode;
                return *this;
            }

            auto setFillMode(FillMode mode) -> Desc&
            {
                m_fillMode = mode;
                return *this;
            }

            auto setFrontCounterCW(bool isFrontCCW) -> Desc&
            {
                m_isFrontCcw = isFrontCCW;
                return *this;
            }

            auto setDepthBias(int32_t depthBias, float slopeScaledBias) -> Desc&
            {
                m_slopeScaledDepthBias = slopeScaledBias;
                m_depthBias = depthBias;
                return *this;
            }

            auto setDepthClamp(bool clampDepth) -> Desc&
            {
                m_clampDepth = clampDepth;
                return *this;
            }

            auto setLineAntiAliasing(bool enable) -> Desc&
            {
                m_enableLinesAA = enable;
                return *this;
            }

            auto setScissorTest(bool enabled) -> Desc&
            {
                m_scissorEnabled = enabled;
                return *this;
            }

            auto setConservativeRasterization(bool enabled) -> Desc&
            {
                m_conservativeRaster = enabled;
                return *this;
            }

            auto setForcedSampleCount(uint32_t samples) -> Desc&
            {
                m_forcedSampleCount = samples;
                return *this;
            }

        protected:
            CullMode m_cullMode{CullMode::Back};
            FillMode m_fillMode{FillMode::Solid};
            bool m_isFrontCcw{true};
            float m_slopeScaledDepthBias{0};
            int32_t m_depthBias{0};
            bool m_clampDepth{false};
            bool m_scissorEnabled{false};
            bool m_enableLinesAA{true};
            uint32_t m_forcedSampleCount{0};
            bool m_conservativeRaster{false};
        };

        virtual ~RasterizerState();

        static auto create(Desc const& desc) -> core::ref<RasterizerState>;

        auto getCullMode() const -> CullMode { return m_desc.m_cullMode; }
        auto getFillMode() const -> FillMode { return m_desc.m_fillMode; }
        auto isFrontCounterCW() const -> bool { return m_desc.m_isFrontCcw; }
        auto getSlopeScaledDepthBias() const -> float { return m_desc.m_slopeScaledDepthBias; }
        auto getDepthBias() const -> int32_t { return m_desc.m_depthBias; }
        auto isDepthClampEnabled() const -> bool { return m_desc.m_clampDepth; }
        auto isScissorTestEnabled() const -> bool { return m_desc.m_scissorEnabled; }
        auto isLineAntiAliasingEnabled() const -> bool { return m_desc.m_enableLinesAA; }
        auto isConservativeRasterizationEnabled() const -> bool { return m_desc.m_conservativeRaster; }
        auto getForcedSampleCount() const -> uint32_t { return m_desc.m_forcedSampleCount; }

    private:
        RasterizerState(Desc const& desc) : m_desc(desc) {}
        Desc m_desc{};
    };

    AP_ENUM_REGISTER(RasterizerState::CullMode);
}
