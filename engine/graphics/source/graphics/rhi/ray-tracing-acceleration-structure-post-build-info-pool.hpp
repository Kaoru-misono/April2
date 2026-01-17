// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "fwd.hpp"

#include <core/foundation/object.hpp>
#include <slang-com-ptr.h>
#include <slang-rhi.h>

namespace april::graphics
{
    enum class RtAccelerationStructurePostBuildInfoQueryType
    {
        CompactedSize,
        SerializationSize,
        CurrentSize,
    };

    class CommandContext;

    class RtAccelerationStructurePostBuildInfoPool : public core::Object
    {
        APRIL_OBJECT(RtAccelerationStructurePostBuildInfoPool)
    public:
        struct Desc
        {
            RtAccelerationStructurePostBuildInfoQueryType type;
            uint32_t elementCount;
        };

        static auto create(core::ref<Device> device, uint32_t elementCount) -> core::ref<RtAccelerationStructurePostBuildInfoPool>;

        ~RtAccelerationStructurePostBuildInfoPool();

        auto getElement(CommandContext* context, uint32_t index) -> uint64_t;
        auto reset(CommandContext* context) -> void;
        auto getGfxQueryPool() const -> rhi::IQueryPool* { return m_gfxQueryPool.get(); }

    protected:
        RtAccelerationStructurePostBuildInfoPool(core::ref<Device> device, Desc const& desc);

    private:
        Desc m_desc{};
        Slang::ComPtr<rhi::IQueryPool> m_gfxQueryPool{};
        bool m_needFlush{true};
    };

    struct RtAccelerationStructurePostBuildInfoDesc
    {
        RtAccelerationStructurePostBuildInfoQueryType type{};
        RtAccelerationStructurePostBuildInfoPool* pool{};
        uint32_t index{};
    };
}
