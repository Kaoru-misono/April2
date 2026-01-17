// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "types.hpp"

#include <core/foundation/object.hpp>
#include <core/math/type.hpp>
#include <cstdint>

namespace april::graphics
{
    class DepthStencilState : public core::Object
    {
        APRIL_OBJECT(DepthStencilState)
    public:
        enum class Face
        {
            Front,
            Back,
            FrontAndBack
        };

        enum class StencilOp
        {
            Keep,
            Zero,
            Replace,
            Increase,
            IncreaseSaturate,
            Decrease,
            DecreaseSaturate,
            Invert
        };

        struct StencilDesc
        {
            ComparisonFunc func{ComparisonFunc::Disabled};
            StencilOp stencilFailOp{StencilOp::Keep};
            StencilOp depthFailOp{StencilOp::Keep};
            StencilOp depthStencilPassOp{StencilOp::Keep};
        };

        class Desc
        {
        public:
            friend class DepthStencilState;

            auto setDepthEnabled(bool enabled) -> Desc&
            {
                m_depthEnabled = enabled;
                return *this;
            }

            auto setDepthFunc(ComparisonFunc func) -> Desc&
            {
                m_depthFunc = func;
                return *this;
            }

            auto setDepthWriteMask(bool write) -> Desc&
            {
                m_writeDepth = write;
                return *this;
            }

            auto setStencilEnabled(bool enabled) -> Desc&
            {
                m_stencilEnabled = enabled;
                return *this;
            }

            auto setStencilWriteMask(uint8_t mask) -> Desc& { m_stencilWriteMask = mask; return *this; }
            auto setStencilReadMask(uint8_t mask) -> Desc& { m_stencilReadMask = mask; return *this; }

            auto setStencilFunc(Face face, ComparisonFunc func) -> Desc&;
            auto setStencilOp(Face face, StencilOp stencilFail, StencilOp depthFail, StencilOp depthStencilPass) -> Desc&;

            auto setStencilRef(uint8_t value) -> Desc&
            {
                m_stencilRef = value;
                return *this;
            }

        protected:
            bool m_depthEnabled{true};
            bool m_stencilEnabled{false};
            bool m_writeDepth{true};
            ComparisonFunc m_depthFunc{ComparisonFunc::Less};
            StencilDesc m_stencilFront;
            StencilDesc m_stencilBack;
            uint8_t m_stencilReadMask{0xff};
            uint8_t m_stencilWriteMask{0xff};
            uint8_t m_stencilRef{0};
        };

        virtual ~DepthStencilState();

        static auto create(Desc const& desc) -> core::ref<DepthStencilState>;

        auto isDepthTestEnabled() const -> bool { return m_desc.m_depthEnabled; }
        auto isDepthWriteEnabled() const -> bool { return m_desc.m_writeDepth; }
        auto getDepthFunc() const -> ComparisonFunc { return m_desc.m_depthFunc; }

        auto isStencilTestEnabled() const -> bool { return m_desc.m_stencilEnabled; }
        auto getStencilDesc(Face face) const -> StencilDesc const&;

        auto getStencilReadMask() const -> uint8_t { return m_desc.m_stencilReadMask; }
        auto getStencilWriteMask() const -> uint8_t { return m_desc.m_stencilWriteMask; }
        auto getStencilRef() const -> uint8_t { return m_desc.m_stencilRef; }

    private:
        DepthStencilState(Desc const& desc) : m_desc(desc) {}
        Desc m_desc{};
    };
}
