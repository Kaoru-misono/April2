#include "inspector-window.hpp"

#include <editor/editor-context.hpp>
#include <editor/ui/ui.hpp>
#include <runtime/engine.hpp>
#include <scene/scene.hpp>
#include <imgui.h>

#include <string>

namespace april::editor
{
    auto InspectorWindow::onUIRender(EditorContext& context) -> void
    {
        if (!open)
        {
            return;
        }

        ui::ScopedWindow window{title(), &open};
        if (!window)
        {
            return;
        }

        auto* sceneGraph = Engine::get().getSceneGraph();
        if (!sceneGraph)
        {
            ImGui::Text("No active scene graph.");
            return;
        }

        auto& registry = sceneGraph->getRegistry();
        auto const selected = context.selection.entity;

        if (selected == scene::NullEntity)
        {
            ImGui::Text("Select an entity to edit.");
            ImGui::Separator();
            ImGui::Text("Project: %s", context.projectName.c_str());
            return;
        }

        if (!registry.allOf<scene::RelationshipComponent>(selected))
        {
            ImGui::Text("Selected entity is not valid.");
            ImGui::Separator();
            ImGui::Text("Project: %s", context.projectName.c_str());
            return;
        }

        if (selected != m_lastEntity)
        {
            m_lastEntity = selected;
            if (registry.allOf<scene::TagComponent>(selected))
            {
                m_tagBuffer = registry.get<scene::TagComponent>(selected).tag;
            }
            else
            {
                m_tagBuffer.clear();
            }

            m_meshAssetBuffer.clear();
            m_materialAssetBuffer.clear();
        }

        if (registry.allOf<scene::TagComponent>(selected))
        {
            auto& tag = registry.get<scene::TagComponent>(selected);
            ui::PropertyUndoable(context, "Tag", m_tagBuffer, "Set Tag",
                [&](std::string const& value) { tag.tag = value; });
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
                bool changed = false;
                changed |= ui::PropertyUndoable(context, "Position", transform, &scene::TransformComponent::localPosition, "Move Entity", 0.1f);
                changed |= ui::PropertyUndoable(context, "Rotation (rad)", transform, &scene::TransformComponent::localRotation, "Rotate Entity", 0.01f);
                changed |= ui::PropertyUndoable(context, "Scale", transform, &scene::TransformComponent::localScale, "Scale Entity", 0.01f, 0.0f, 0.0f, 1.0f);
                if (changed && sceneGraph)
                {
                    sceneGraph->markTransformDirty(selected);
                }
            }
        }

        if (registry.allOf<scene::MeshRendererComponent>(selected))
        {
            if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& meshRenderer = registry.get<scene::MeshRendererComponent>(selected);
                if (auto* resources = Engine::get().getRenderResourceRegistry())
                {
                    ui::PropertyUndoable(context, "Mesh Asset", m_meshAssetBuffer, "Set Mesh Asset",
                        [&](std::string const& value) {
                            m_meshAssetBuffer = value;
                            if (value.empty())
                            {
                                meshRenderer.meshId = scene::kInvalidRenderID;
                                return;
                            }
                            meshRenderer.meshId = resources->registerMesh(value);
                            if (meshRenderer.materialId == scene::kInvalidRenderID)
                            {
                                meshRenderer.materialId = resources->getMeshMaterialId(meshRenderer.meshId, 0);
                            }
                        });

                    ui::PropertyUndoable(context, "Material Asset", m_materialAssetBuffer, "Set Material Asset",
                        [&](std::string const& value) {
                            m_materialAssetBuffer = value;
                            if (value.empty())
                            {
                                meshRenderer.materialId = scene::kInvalidRenderID;
                                return;
                            }
                            meshRenderer.materialId = resources->getOrCreateMaterialId(value);
                        });
                }

                ImGui::Text("Mesh ID: %u", meshRenderer.meshId);
                ImGui::Text("Material ID: %u", meshRenderer.materialId);
                if (auto* resources = Engine::get().getRenderResourceRegistry())
                {
                    auto const gpuMaterialIndex = resources->getMaterialBufferIndex(meshRenderer.materialId);
                    auto const materialTypeId = resources->getMaterialTypeId(meshRenderer.materialId);
                    auto const materialTypeName = resources->getMaterialTypeName(meshRenderer.materialId);
                    ImGui::Text("GPU Material Index: %u", gpuMaterialIndex);
                    ImGui::Text("Material Type: %s (%u)", materialTypeName.c_str(), materialTypeId);
                }
                ui::PropertyUndoable(context, "Enabled", meshRenderer, &scene::MeshRendererComponent::enabled, "Toggle Mesh Renderer");
                ui::PropertyUndoable(context, "Cast Shadows", meshRenderer, &scene::MeshRendererComponent::castShadows, "Toggle Cast Shadows");
                ui::PropertyUndoable(context, "Receive Shadows", meshRenderer, &scene::MeshRendererComponent::receiveShadows, "Toggle Receive Shadows");
            }
        }

        if (registry.allOf<scene::CameraComponent>(selected))
        {
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& camera = registry.get<scene::CameraComponent>(selected);
                ui::PropertyUndoable(context, "Perspective", camera, &scene::CameraComponent::isPerspective, "Toggle Projection");

                if (camera.isPerspective)
                {
                    ui::PropertyUndoable(context, "FOV", camera, &scene::CameraComponent::fov, "Set FOV", 0.01f, 0.1f, 3.1f);
                }
                else
                {
                    ui::PropertyUndoable(context, "Size", camera, &scene::CameraComponent::orthoSize, "Set Ortho Size", 0.1f);
                }

                ui::PropertyUndoable(context, "Near Clip", camera, &scene::CameraComponent::nearClip, "Set Near Clip", 0.1f);
                ui::PropertyUndoable(context, "Far Clip", camera, &scene::CameraComponent::farClip, "Set Far Clip", 1.0f);
            }
        }

        if (ImGui::CollapsingHeader("Relationship", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto const& relationship = registry.get<scene::RelationshipComponent>(selected);
            auto const parentIndex = relationship.parent == scene::NullEntity ? 0u : relationship.parent.index;
            ImGui::Text("Parent: %u", parentIndex);
            ImGui::Text("Children: %zu", relationship.childrenCount);
        }

        ImGui::Separator();
        ImGui::Text("Project: %s", context.projectName.c_str());
    }
}
