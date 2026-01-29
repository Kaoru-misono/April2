#include "editor-viewport.hpp"

#include <runtime/engine.hpp>
#include <imgui.h>

namespace april::editor
{
    auto EditorViewportElement::onAttach(ui::ImGuiLayer* /*pLayer*/) -> void {}
    auto EditorViewportElement::onDetach() -> void {}
    auto EditorViewportElement::onUIMenu() -> void {}
    auto EditorViewportElement::onPreRender() -> void {}
    auto EditorViewportElement::onRender(graphics::CommandContext* /*pContext*/) -> void {}
    auto EditorViewportElement::onFileDrop(std::filesystem::path const& /*filename*/) -> void {}

    auto EditorViewportElement::onResize(graphics::CommandContext* /*pContext*/, float2 const& size) -> void
    {
        m_context.viewportSize = size;
        auto width = static_cast<uint32_t>(size.x);
        auto height = static_cast<uint32_t>(size.y);
        if (width > 0 && height > 0)
        {
            Engine::get().setSceneViewportSize(width, height);
        }
    }

    auto EditorViewportElement::onUIRender() -> void
    {
        ImGui::Begin("Viewport");
        auto size = ImGui::GetContentRegionAvail();
        auto sceneSrv = Engine::get().getSceneColorSrv();
        if (sceneSrv)
        {
            ImGui::Image(
                (ImTextureID)sceneSrv.get(),
                size,
                ImVec2(0.0f, 0.0f),
                ImVec2(1.0f, 1.0f)
            );
        }
        else
        {
            ImGui::Text("Viewport: %.0f x %.0f", m_context.viewportSize.x, m_context.viewportSize.y);
        }
        ImGui::End();
    }
}
