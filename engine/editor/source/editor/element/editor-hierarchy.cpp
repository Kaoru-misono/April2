#include "editor-hierarchy.hpp"

#include <runtime/engine.hpp>
#include <scene/scene.hpp>
#include <imgui.h>

#include <format>
#include <cstdint>

namespace april::editor
{
    auto EditorHierarchyElement::onAttach(ui::ImGuiLayer* /*pLayer*/) -> void {}
    auto EditorHierarchyElement::onDetach() -> void {}
    auto EditorHierarchyElement::onResize(graphics::CommandContext* /*pContext*/, float2 const& /*size*/) -> void {}
    auto EditorHierarchyElement::onUIMenu() -> void {}
    auto EditorHierarchyElement::onPreRender() -> void {}
    auto EditorHierarchyElement::onRender(graphics::CommandContext* /*pContext*/) -> void {}
    auto EditorHierarchyElement::onFileDrop(std::filesystem::path const& /*filename*/) -> void {}

    namespace
    {
        auto getEntityLabel(scene::Registry const& registry, scene::Entity entity) -> std::string
        {
            if (registry.allOf<scene::TagComponent>(entity))
            {
                return registry.get<scene::TagComponent>(entity).tag;
            }
            return std::format("Entity {}", entity);
        }

        auto drawEntityNode(EditorContext& context, scene::Registry const& registry, scene::Entity entity) -> void
        {
            if (!registry.allOf<scene::RelationshipComponent>(entity))
            {
                return;
            }

            auto const& relationship = registry.get<scene::RelationshipComponent>(entity);
            auto const hasChildren = relationship.firstChild != scene::NullEntity;

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow;
            if (context.selection.entity == entity)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            if (!hasChildren)
            {
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            }

            auto const label = getEntityLabel(registry, entity);
            bool opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(entity)), flags, "%s", label.c_str());

            if (ImGui::IsItemClicked())
            {
                context.selection.entity = entity;
            }

            if (opened && hasChildren)
            {
                auto child = relationship.firstChild;
                while (child != scene::NullEntity)
                {
                    drawEntityNode(context, registry, child);
                    auto const& childRel = registry.get<scene::RelationshipComponent>(child);
                    child = childRel.nextSibling;
                }
                ImGui::TreePop();
            }
        }
    }

    auto EditorHierarchyElement::onUIRender() -> void
    {
        ImGui::Begin("Hierarchy");
        if (ImGui::Selectable(m_context.scene.name.c_str(), m_context.selection.entity == scene::NullEntity))
        {
            m_context.selection.entity = scene::NullEntity;
        }

        auto* sceneGraph = Engine::get().getSceneGraph();
        if (!sceneGraph)
        {
            ImGui::Text("No active scene graph.");
            ImGui::End();
            return;
        }

        auto const& registry = sceneGraph->getRegistry();
        auto const* relationshipPool = registry.getPool<scene::RelationshipComponent>();
        if (!relationshipPool)
        {
            ImGui::Text("No entities.");
            ImGui::End();
            return;
        }

        for (size_t i = 0; i < relationshipPool->data().size(); ++i)
        {
            auto const entity = relationshipPool->getEntity(i);
            auto const& relationship = relationshipPool->data()[i];
            if (relationship.parent == scene::NullEntity)
            {
                drawEntityNode(m_context, registry, entity);
            }
        }

        ImGui::End();
    }
}
