#include "editor-inspector.hpp"

#include <runtime/engine.hpp>
#include <scene/scene.hpp>
#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace april::editor
{
    auto EditorInspectorElement::onAttach(ui::ImGuiLayer* /*pLayer*/) -> void {}
    auto EditorInspectorElement::onDetach() -> void {}
    auto EditorInspectorElement::onResize(graphics::CommandContext* /*pContext*/, float2 const& /*size*/) -> void {}
    auto EditorInspectorElement::onUIMenu() -> void {}
    auto EditorInspectorElement::onPreRender() -> void {}
    auto EditorInspectorElement::onRender(graphics::CommandContext* /*pContext*/) -> void {}
    auto EditorInspectorElement::onFileDrop(std::filesystem::path const& /*filename*/) -> void {}

    namespace
    {
        auto copyToBuffer(std::array<char, 128>& buffer, std::string const& value) -> void
        {
            std::snprintf(buffer.data(), buffer.size(), "%s", value.c_str());
        }

        auto copyToBuffer(std::array<char, 256>& buffer, std::string const& value) -> void
        {
            std::snprintf(buffer.data(), buffer.size(), "%s", value.c_str());
        }
    }

    auto EditorInspectorElement::onUIRender() -> void
    {
        ImGui::Begin("Inspector");

        auto* sceneGraph = Engine::get().getSceneGraph();
        if (!sceneGraph)
        {
            ImGui::Text("No active scene graph.");
            ImGui::End();
            return;
        }

        auto& registry = sceneGraph->getRegistry();
        auto const selected = m_context.selection.entity;

        if (selected == scene::NullEntity)
        {
            ImGui::Text("Select an entity to edit.");
            ImGui::Separator();
            ImGui::Text("Project: %s", m_context.projectName.c_str());
            ImGui::End();
            return;
        }

        if (!registry.allOf<scene::RelationshipComponent>(selected))
        {
            ImGui::Text("Selected entity is not valid.");
            ImGui::Separator();
            ImGui::Text("Project: %s", m_context.projectName.c_str());
            ImGui::End();
            return;
        }

        if (selected != m_lastEntity)
        {
            m_lastEntity = selected;
            if (registry.allOf<scene::TagComponent>(selected))
            {
                copyToBuffer(m_tagBuffer, registry.get<scene::TagComponent>(selected).tag);
            }
            else
            {
                m_tagBuffer[0] = '\0';
            }

            if (registry.allOf<scene::MeshRendererComponent>(selected))
            {
                copyToBuffer(m_meshBuffer, registry.get<scene::MeshRendererComponent>(selected).meshAssetPath);
            }
            else
            {
                m_meshBuffer[0] = '\0';
            }
        }

        if (registry.allOf<scene::TagComponent>(selected))
        {
            auto& tag = registry.get<scene::TagComponent>(selected);
            if (ImGui::InputText("Tag", m_tagBuffer.data(), m_tagBuffer.size()))
            {
                tag.tag = m_tagBuffer.data();
            }
        }

        if (registry.allOf<scene::IDComponent>(selected))
        {
            auto const& id = registry.get<scene::IDComponent>(selected).id;
            ImGui::Text("UUID: %s", id.toString().c_str());
        }

        if (registry.allOf<scene::TransformComponent>(selected))
        {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& transform = registry.get<scene::TransformComponent>(selected);
                if (ImGui::DragFloat3("Position", &transform.localPosition.x, 0.1f))
                {
                    transform.isDirty = true;
                }
                if (ImGui::DragFloat3("Rotation (rad)", &transform.localRotation.x, 0.01f))
                {
                    transform.isDirty = true;
                }
                if (ImGui::DragFloat3("Scale", &transform.localScale.x, 0.01f))
                {
                    transform.isDirty = true;
                }
            }
        }

        if (registry.allOf<scene::MeshRendererComponent>(selected))
        {
            if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& meshRenderer = registry.get<scene::MeshRendererComponent>(selected);
                if (ImGui::InputText("Mesh Asset", m_meshBuffer.data(), m_meshBuffer.size()))
                {
                    meshRenderer.meshAssetPath = m_meshBuffer.data();
                }
                ImGui::Checkbox("Enabled", &meshRenderer.enabled);
                ImGui::Checkbox("Cast Shadows", &meshRenderer.castShadows);
                ImGui::Checkbox("Receive Shadows", &meshRenderer.receiveShadows);
            }
        }

        if (registry.allOf<scene::CameraComponent>(selected))
        {
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& camera = registry.get<scene::CameraComponent>(selected);
                if (ImGui::Checkbox("Perspective", &camera.isPerspective))
                {
                    camera.isDirty = true;
                }
                if (ImGui::DragFloat("FOV", &camera.fov, 0.01f, 0.1f, 3.1f))
                {
                    camera.isDirty = true;
                }
                if (ImGui::DragFloat("Ortho Size", &camera.orthoSize, 0.1f, 0.1f, 1000.0f))
                {
                    camera.isDirty = true;
                }
                if (ImGui::DragFloat("Near", &camera.nearClip, 0.01f, 0.001f, 1000.0f))
                {
                    camera.isDirty = true;
                }
                if (ImGui::DragFloat("Far", &camera.farClip, 1.0f, 1.0f, 10000.0f))
                {
                    camera.isDirty = true;
                }
            }
        }

        if (ImGui::CollapsingHeader("Relationship", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto const& relationship = registry.get<scene::RelationshipComponent>(selected);
            ImGui::Text("Parent: %u", relationship.parent == scene::NullEntity ? 0u : relationship.parent);
            ImGui::Text("Children: %zu", relationship.childrenCount);
        }

        ImGui::Separator();
        ImGui::Text("Project: %s", m_context.projectName.c_str());
        ImGui::End();
    }
}
