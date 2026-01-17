// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "fwd.hpp"
#include "handles.hpp"
#include "native-handle.hpp"

#include <core/foundation/object.hpp>
#include <core/error/assert.hpp>
#include <slang-rhi.h>
#include <limits>

namespace april::graphics
{
    struct FenceDesc
    {
        bool initialValue{0};
        bool shared{false};
    };

    /**
     * This class represents a fence on the device.
     * It is used to synchronize host and device execution.
     * On the device, the fence is represented by a 64-bit integer.
     * On the host, we keep a copy of the last signaled value.
     * By default, the fence value is monotonically incremented every time it is signaled.
     *
     * To synchronize the host with the device, we can do the following:
     *
     * ref<Fence> fence = device->createFence();
     * <schedule device work 1>
     * // Signal the fence once we have finished all the above work on the device.
     * commandContext->signal(fence);
     * <schedule device work 2>
     * // Wait on the host until <device work 1> is finished.
     * fence->wait();
     */
    class Fence : public core::Object
    {
        APRIL_OBJECT(Fence)
    public:
        static constexpr uint64_t kAuto = std::numeric_limits<uint64_t>::max();
        static constexpr uint64_t kTimeoutInfinite = std::numeric_limits<uint64_t>::max();

        /// Constructor.
        /// Do not use directly, use Device::createFence instead.
        Fence(core::ref<Device> const& p_device, FenceDesc desc);

        virtual ~Fence();

        /// Returns the description.
        auto getDesc() const -> FenceDesc const& { return m_desc; }

        /**
         * Signal the fence.
         * This signals the fence from the host.
         * @param value The value to signal. If kAuto, the signaled value will be auto-incremented.
         * @return Returns the signaled value.
         */
        auto signal(uint64_t value = kAuto) -> uint64_t;

        /**
         * Wait for the fence to be signaled on the host.
         * Blocks the host until the fence reaches or exceeds the specified value.
         * @param value The value to wait for. If kAuto, wait for the last signaled value.
         * @param timeoutNs The timeout in nanoseconds. If kTimeoutInfinite, the function will block indefinitely.
         */
        auto wait(uint64_t value = kAuto, uint64_t timeoutNs = kTimeoutInfinite) -> void;

        /// Returns the current value on the device.
        auto getCurrentValue() -> uint64_t;

        /// Returns the latest signaled value (after auto-increment).
        auto getSignaledValue() const -> uint64_t { return m_signaledValue; }

        /**
         * Updates or increments the signaled value.
         * This is used before signaling a fence (from the host, on the device or
         * from an external source), to update the internal state.
         * The passed value is stored, or if value == kAuto, the last signaled
         * value is auto-incremented by one. The returned value is what the caller
         * should signal to the fence.
         * @param value The value to signal. If kAuto, the signaled value will be auto-incremented.
         * @return Returns the value to signal to the fence.
         */
        auto updateSignaledValue(uint64_t value = kAuto) -> uint64_t;

        /**
         * Get the internal API handle
         */
        auto getGfxFence() const -> rhi::IFence* { return m_gfxFence.get(); }

        /**
         * Returns the native API handle:
         * - D3D12: ID3D12Fence*
         * - Vulkan: currently not supported
         */
        auto getNativeHandle() const -> rhi::NativeHandle;

        /**
         * Creates a shared fence API handle.
         */
        auto getSharedApiHandle() const -> SharedResourceApiHandle;

        auto getDevice() const -> Device* { return m_device.get(); }

        auto breakStrongReferenceToDevice() -> void;

    private:
        core::BreakableReference<Device> m_device;
        FenceDesc m_desc{};
        rhi::ComPtr<rhi::IFence> m_gfxFence{};
        uint64_t m_signaledValue{0};
    };
} // namespace april::graphics
