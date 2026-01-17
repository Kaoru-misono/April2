// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "fence.hpp"

#include <core/foundation/object.hpp>
#include <slang-com-ptr.h>

namespace april::graphics
{
    class Device;

    class LowLevelContextData
    {
    public:
        LowLevelContextData(Device* device, rhi::ICommandQueue* queue);
        ~LowLevelContextData();

        auto getGfxCommandQueue() const -> rhi::ICommandQueue* { return mp_gfxCommandQueue; }
        auto getGfxCommandEncoder() const -> rhi::ICommandEncoder* { return m_gfxEncoder; }

        auto getCommandQueueNativeHandle() const -> rhi::NativeHandle;
        auto getCommandBufferNativeHandle() const -> rhi::NativeHandle;

        auto getFence() const -> core::ref<Fence> const& { return mp_fence; }

        auto submitCommandBuffer() -> void;

    private:
        Device* mp_device{};
        rhi::ICommandQueue* mp_gfxCommandQueue{};
        rhi::ComPtr<rhi::ICommandEncoder> m_gfxEncoder{};
        rhi::ComPtr<rhi::ICommandBuffer> mp_commandBuffer{};
        core::ref<Fence> mp_fence{};
    };
}
