// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "texture.hpp"

#include <core/foundation/object.hpp>
#include <slang-com-ptr.h>

namespace april::graphics
{
    class Device;

    using WindowHandle = void*;

    class Swapchain : public core::Object
    {
        APRIL_OBJECT(Swapchain)
    public:
        struct Desc
        {
            ResourceFormat format{ResourceFormat::Unknown};
            uint32_t width{0};
            uint32_t height{0};
            uint32_t imageCount{3};
            bool enableVSync{false};
        };

        Swapchain(core::ref<Device> device, Desc const& desc, WindowHandle windowHandle);

        auto getDesc() const -> Desc const& { return m_desc; }

        auto present() -> void;
        auto acquireNextImage() -> core::ref<Texture>;
        auto resize(uint32_t width, uint32_t height) -> void;

        auto getGfxSurface() const -> rhi::ISurface* { return m_gfxSurface; }

    private:
        auto configure() -> bool;

    private:
        core::ref<Device> mp_device{};
        Desc m_desc{};
        rhi::ComPtr<rhi::ISurface> m_gfxSurface{};
        core::ref<Texture> m_currentFrameBackBuffer{};
    };
}
