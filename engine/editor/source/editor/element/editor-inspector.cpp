#include "editor-inspector.hpp"

#include <imgui.h>

namespace april::editor
{
    auto EditorInspectorElement::onAttach(ui::ImGuiLayer* /*pLayer*/) -> void {}
    auto EditorInspectorElement::onDetach() -> void {}
    auto EditorInspectorElement::onResize(graphics::CommandContext* /*pContext*/, float2 const& /*size*/) -> void {}
    auto EditorInspectorElement::onUIMenu() -> void {}
    auto EditorInspectorElement::onPreRender() -> void {}
    auto EditorInspectorElement::onRender(graphics::CommandContext* /*pContext*/) -> void {}
    auto EditorInspectorElement::onFileDrop(std::filesystem::path const& /*filename*/) -> void {}

    auto EditorInspectorElement::onUIRender() -> void
    {
        ImGui::Begin("Inspector");
        if (m_context.selection.entityName.empty())
        {
            ImGui::Text("Select an entity to edit.");
        }
        else
        {
            ImGui::Text("Selected: %s", m_context.selection.entityName.c_str());
        }
        ImGui::Separator();
        ImGui::Text("Project: %s", m_context.projectName.c_str());
        ImGui::End();
    }
}
