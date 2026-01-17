// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "low-level-context-data.hpp"
#include "render-device.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>

namespace april::graphics
{
    LowLevelContextData::LowLevelContextData(Device* device, rhi::ICommandQueue* queue) : mp_device(device), mp_gfxCommandQueue(queue)
    {
        mp_fence = mp_device->createFence();
        mp_fence->breakStrongReferenceToDevice();

        m_gfxEncoder = mp_gfxCommandQueue->createCommandEncoder();
    }

    LowLevelContextData::~LowLevelContextData()
    {
    }

    auto LowLevelContextData::getCommandQueueNativeHandle() const -> rhi::NativeHandle
    {
        rhi::NativeHandle gfxNativeHandle = {};
        checkResult(mp_gfxCommandQueue->getNativeHandle(&gfxNativeHandle), "Failed to get command queue native handle");

        return gfxNativeHandle;
    }

    auto LowLevelContextData::getCommandBufferNativeHandle() const -> rhi::NativeHandle
    {
        rhi::NativeHandle gfxNativeHandle = {};
        checkResult(mp_commandBuffer->getNativeHandle(&gfxNativeHandle), "Failed to get command buffer native handle");

        return gfxNativeHandle;
    }

    auto LowLevelContextData::submitCommandBuffer() -> void
    {
        checkResult(m_gfxEncoder->finish(mp_commandBuffer.writeRef()), "Failed to close command buffer");

        rhi::ICommandBuffer* commandBuffer = mp_commandBuffer;

        rhi::IFence* signalFence = mp_fence->getGfxFence();
        uint64_t signalValue = mp_fence->updateSignaledValue(mp_fence->getSignaledValue() + 1);

        rhi::SubmitDesc submitDesc = {};
        submitDesc.commandBuffers = &commandBuffer;
        submitDesc.commandBufferCount = 1;
        submitDesc.signalFences = &signalFence;
        submitDesc.signalFenceValues = &signalValue;
        submitDesc.signalFenceCount = 1;

        checkResult(mp_gfxCommandQueue->submit(submitDesc), "Failed to submit command buffer");

        mp_commandBuffer = {};
        m_gfxEncoder = mp_gfxCommandQueue->createCommandEncoder();
    }
} // namespace april::graphics
