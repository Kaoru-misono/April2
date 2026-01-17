#include "imgui-pass.hpp"
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>

namespace april::graphics
{
    ImGuiPass::ImGuiPass(core::ref<Device> pDevice, core::ref<ImGuiLayer> pLayer)
        : mp_device(pDevice)
        , mp_layer(pLayer)
    {
    }

    auto ImGuiPass::execute(CommandContext* pCtx, core::ref<RenderTargetView> pTargetView) -> void
    {
        if (mp_layer)
        {
            mp_layer->begin();
            // User UI code will be called here usually, but for the pass itself
            // it just manages the begin/end lifecycle.
            mp_layer->end(pCtx, pTargetView.get());
        }
    }
}
