// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "fence.hpp"
#include "render-device.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>

namespace april::graphics
{
    Fence::Fence(core::ref<Device> const& p_device, FenceDesc desc) : m_device(p_device), m_desc(desc)
    {
        AP_ASSERT(m_device);
        rhi::FenceDesc gfxDesc = {};
        m_signaledValue = m_desc.initialValue;
        gfxDesc.isShared = m_desc.shared;
        checkResult(m_device->getGfxDevice()->createFence(gfxDesc, m_gfxFence.writeRef()), "Failed to create fence");
    }

    Fence::~Fence() = default;

    auto Fence::signal(uint64_t value) -> uint64_t
    {
        uint64_t signalValue = updateSignaledValue(value);
        checkResult(m_gfxFence->setCurrentValue(signalValue), "Failed to signal fence");
        return signalValue;
    }

    auto Fence::wait(uint64_t value, uint64_t timeoutNs) -> void
    {
        uint64_t waitValue = value == kAuto ? m_signaledValue : value;
        uint64_t currentValue = getCurrentValue();
        if (currentValue >= waitValue)
        {
            return;
        }
        rhi::IFence* fences[] = {m_gfxFence.get()};
        uint64_t waitValues[] = {waitValue};
        checkResult(m_device->getGfxDevice()->waitForFences(1, fences, waitValues, true, timeoutNs), "Failed to wait for fence");
    }

    auto Fence::getCurrentValue() -> uint64_t
    {
        uint64_t value;
        checkResult(m_gfxFence->getCurrentValue(&value), "Failed to get current fence value");
        return value;
    }

    auto Fence::updateSignaledValue(uint64_t value) -> uint64_t
    {
        m_signaledValue = value == kAuto ? m_signaledValue + 1 : value;
        return m_signaledValue;
    }

    auto Fence::getSharedApiHandle() const -> SharedResourceApiHandle
    {
        rhi::NativeHandle sharedHandle;
        checkResult(m_gfxFence->getSharedHandle(&sharedHandle), "Failed to get shared handle");
        return (SharedResourceApiHandle)sharedHandle.value;
    }

    auto Fence::getNativeHandle() const -> rhi::NativeHandle
    {
        rhi::NativeHandle gfxNativeHandle = {};
        checkResult(m_gfxFence->getNativeHandle(&gfxNativeHandle), "Failed to get native handle");

        return gfxNativeHandle;
    }

    auto Fence::breakStrongReferenceToDevice() -> void
    {
        m_device.breakStrongReference();
    }
} // namespace april::graphics
