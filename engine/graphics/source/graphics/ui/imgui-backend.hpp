#pragma once

#include <core/foundation/object.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/render-target.hpp>

struct ImDrawData;

namespace april::graphics
{
    class Device;
    class RenderPassEncoder;

    /**
     * @brief Interface for ImGui RHI backends.
     */
    class IImGuiBackend
    {
    public:
        virtual ~IImGuiBackend() = default;

        /**
         * @brief Initialize the backend.
         * @param pDevice The render device to use.
         * @param dpiScale The initial DPI scale factor.
         */
        virtual auto init(core::ref<Device> pDevice, float dpiScale = 1.0f) -> void = 0;

        /**
         * @brief Shutdown the backend and release resources.
         */
        virtual auto shutdown() -> void = 0;

        /**
         * @brief Prepare for a new frame.
         */
        virtual auto newFrame() -> void = 0;

        /**
         * @brief Render ImGui draw data.
         * @param drawData The ImGui draw data to render.
         * @param pEncoder The render pass encoder to record commands into.
         */
        virtual auto renderDrawData(ImDrawData* drawData, core::ref<RenderPassEncoder> pEncoder) -> void = 0;
    };
}
