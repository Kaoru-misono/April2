#include "editor-layer.hpp"

#include <imgui.h>

namespace april::editor
{
    auto EditorLayer::onAttach(ui::ImGuiLayer* /*pLayer*/) -> void {}

    auto EditorLayer::onDetach() -> void {}

    auto EditorLayer::onResize(graphics::CommandContext* /*pContext*/, float2 const& size) -> void
    {
        m_viewportSize = size;
    }

    auto EditorLayer::onUIRender() -> void
    {
        ImGui::Begin("Hierarchy");
        ImGui::Text("Scene");
        ImGui::End();

        ImGui::Begin("Inspector");
        ImGui::Text("Select an entity to edit.");
        ImGui::End();

        ImGui::Begin("Viewport");
        ImGui::Text("Viewport: %.0f x %.0f", m_viewportSize.x, m_viewportSize.y);
        ImGui::End();
    }

    auto EditorLayer::onUIMenu() -> void
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                if (m_onExit)
                {
                    m_onExit();
                }
            }
            ImGui::EndMenu();
        }
    }

    auto EditorLayer::onPreRender() -> void {}

    auto EditorLayer::onRender(graphics::CommandContext* /*pContext*/) -> void {}

    auto EditorLayer::onFileDrop(std::filesystem::path const& /*filename*/) -> void {}
}
