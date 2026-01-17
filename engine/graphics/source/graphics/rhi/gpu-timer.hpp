// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "buffer.hpp"

#include <core/foundation/object.hpp>

namespace april::graphics
{
    class Device;

    class GpuTimer : public core::Object
    {
        APRIL_OBJECT(GpuTimer)
    public:
        static auto create(core::ref<Device> device) -> core::ref<GpuTimer>;
        virtual ~GpuTimer();

        auto begin() -> void;
        auto end() -> void;
        auto resolve() -> void;
        auto getElapsedTime() -> double;

        auto breakStrongReferenceToDevice() -> void;

    private:
        GpuTimer(core::ref<Device> device);

        enum class Status
        {
            Begin,
            End,
            Idle
        };

        core::BreakableReference<Device> mp_device;
        Status m_status{Status::Idle};
        uint32_t m_start{0};
        uint32_t m_end{0};
        double m_elapsedTime{0.0};
        bool m_dataPending{false};

        core::ref<Buffer> mp_resolveBuffer{};
        core::ref<Buffer> mp_resolveStagingBuffer{};
    };
}
