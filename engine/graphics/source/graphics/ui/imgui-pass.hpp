#pragma once

#include "imgui-layer.hpp"
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/render-target.hpp>

namespace april::graphics
{
    /**
     * @brief A specialized pass for rendering ImGui.
     */
    class ImGuiPass final : public core::Object
    {
        APRIL_OBJECT(ImGuiPass)
    public:
        ImGuiPass(core::ref<Device> pDevice, core::ref<ImGuiLayer> pLayer);
        ~ImGuiPass() override = default;

        /**
         * @brief Execute the ImGui pass.
         * @param pCtx The command context to record commands into.
         * @param pTargetView The render target view to render into.
         */
        auto execute(CommandContext* pCtx, core::ref<RenderTargetView> pTargetView) -> void;

    private:
        core::ref<Device> mp_device;
        core::ref<ImGuiLayer> mp_layer;
    };
}
