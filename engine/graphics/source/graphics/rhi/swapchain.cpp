// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "swapchain.hpp"
#include "render-device.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>

namespace april::graphics
{
    Swapchain::Swapchain(core::ref<Device> device, Desc const& desc, WindowHandle windowHandle)
        : mp_device(device), m_desc(desc)
    {
        AP_ASSERT(mp_device);

        rhi::WindowHandle gfxWindowHandle = {};
#if defined(_WIN32)
        gfxWindowHandle = rhi::WindowHandle::fromHwnd(windowHandle);
#else
        // TODO: Handle other platforms
#endif
        checkResult(mp_device->getGfxDevice()->createSurface(gfxWindowHandle, m_gfxSurface.writeRef()), "Failed to create swapchain");

        AP_ASSERT(desc.format != ResourceFormat::Unknown, "Invalid format");
        AP_ASSERT(desc.width > 0, "Invalid width");
        AP_ASSERT(desc.height > 0, "Invalid height");
        AP_ASSERT(desc.imageCount > 0, "Invalid image count");

        rhi::SurfaceConfig surfaceConfig = {};
        surfaceConfig.format            = getGFXFormat(desc.format);
        surfaceConfig.width             = desc.width;
        surfaceConfig.height            = desc.height;
        surfaceConfig.desiredImageCount = desc.imageCount;
        surfaceConfig.vsync             = desc.enableVSync;

        m_gfxSurface->configure(surfaceConfig);
    }

    auto Swapchain::present() -> void
    {
        checkResult(m_gfxSurface->present(), "Failed to present swapchain");
    }

    auto Swapchain::acquireNextImage() -> core::ref<Texture>
    {
        auto resource = rhi::ComPtr<rhi::ITexture>{};
        auto result = m_gfxSurface->acquireNextImage(resource.writeRef());
        if (result != SLANG_OK) {
            AP_ERROR("Swapchain::acquireNextImage failed to get resource from surface: {}", result);
            return nullptr;
        }
        m_currentFrameBackBuffer = mp_device->createTextureFromResource(
            resource.get(),
            Resource::Type::Texture2D,
            m_desc.format,
            m_desc.width,
            m_desc.height,
            1,
            1,
            1,
            1,
            TextureUsage::RenderTarget,
            Resource::State::Undefined
        );

        return m_currentFrameBackBuffer;
    }

    auto Swapchain::resize(uint32_t width, uint32_t height) -> void
    {
        if (!m_currentFrameBackBuffer || (width == m_desc.width && height == m_desc.height)) return;

        AP_ASSERT(width > 0);
        AP_ASSERT(height > 0);
        m_desc.width  = width;
        m_desc.height = height;
        m_currentFrameBackBuffer = nullptr;
        m_dirty = true;

        configure();
    }

    auto Swapchain::configure() -> void
    {
        mp_device->wait();

        rhi::SurfaceConfig surfaceConfig = {};
        surfaceConfig.format            = getGFXFormat(m_desc.format);
        surfaceConfig.width             = m_desc.width;
        surfaceConfig.height            = m_desc.height;
        surfaceConfig.desiredImageCount = m_desc.imageCount;
        surfaceConfig.vsync             = m_desc.enableVSync;

        m_gfxSurface->configure(surfaceConfig);
    }
} // namespace april::graphics
