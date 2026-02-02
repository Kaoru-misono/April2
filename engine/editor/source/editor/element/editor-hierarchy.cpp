#include "editor-hierarchy.hpp"

#include <imgui.h>

namespace april::editor
{
    auto EditorHierarchyElement::onAttach(ui::ImGuiLayer* /*pLayer*/) -> void {}
    auto EditorHierarchyElement::onDetach() -> void {}
    auto EditorHierarchyElement::onResize(graphics::CommandContext* /*pContext*/, float2 const& /*size*/) -> void {}
    auto EditorHierarchyElement::onUIMenu() -> void {}
    auto EditorHierarchyElement::onPreRender() -> void {}
    auto EditorHierarchyElement::onRender(graphics::CommandContext* /*pContext*/) -> void {}
    auto EditorHierarchyElement::onFileDrop(std::filesystem::path const& /*filename*/) -> void {}

    auto EditorHierarchyElement::onUIRender() -> void
    {
        ImGui::Begin("Hierarchy");
        char const* sceneName = m_context.scene.name.c_str();
        bool selected = (m_context.selection.entityName == sceneName);
        if (ImGui::Selectable(sceneName, selected))
        {
            m_context.selection.entityName = sceneName;
        }
        ImGui::End();
    }
}
