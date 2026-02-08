#include "viewport-window.hpp"

#include <editor/editor-context.hpp>
#include <editor/ui/ui.hpp>
#include <runtime/engine.hpp>
#include <scene/scene.hpp>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace april::editor
{
    ViewportWindow::~ViewportWindow()
    {
        auto* scene = Engine::get().getSceneGraph();
        if (!scene)
        {
            return;
        }

        if (m_cameraEntity != scene::NullEntity)
        {
            scene->destroyEntity(m_cameraEntity);
            m_cameraEntity = scene::NullEntity;
        }
    }

    auto ViewportWindow::ensureScene(EditorContext& /*context*/) -> void
    {
        if (m_camera)
        {
            return;
        }

        auto constexpr kDefaultFov = 45.0f;
        m_camera = std::make_unique<EditorCamera>(glm::radians(kDefaultFov), 1.777f, 0.1f, 1000.0f);
        m_camera->setPosition(float3{0.0f, 0.0f, 10.0f});

        auto* scene = Engine::get().getSceneGraph();
        if (!scene)
        {
            return;
        }

        m_cameraEntity = scene->createEntity("MainCamera");
        auto& registry = scene->getRegistry();
        auto& cameraComponent = registry.emplace<scene::CameraComponent>(m_cameraEntity);
        cameraComponent.isPerspective = true;
        cameraComponent.fov = glm::radians(kDefaultFov);
        cameraComponent.nearClip = 0.1f;
        cameraComponent.farClip = 1000.0f;

        auto& transform = registry.get<scene::TransformComponent>(m_cameraEntity);
        transform.localPosition = m_camera->getPosition();
        transform.isDirty = true;
        scene->markTransformDirty(m_cameraEntity);

        auto const* meshPool = registry.getPool<scene::MeshRendererComponent>();
        if (!meshPool || meshPool->data().empty())
        {
            auto cube = scene->createEntity("Cube");
            auto& meshRenderer = registry.emplace<scene::MeshRendererComponent>(cube);
            if (auto* resources = Engine::get().getRenderResourceRegistry())
            {
                meshRenderer.meshId = resources->registerMesh("E:/github/April2/content/model/cube.gltf.asset");
                if (meshRenderer.materialId == scene::kInvalidRenderID)
                {
                    meshRenderer.materialId = resources->getMeshMaterialId(meshRenderer.meshId, 0);
                }
            }
            meshRenderer.enabled = true;

            auto& cubeTransform = registry.get<scene::TransformComponent>(cube);
            cubeTransform.localPosition = {0.0f, 0.0f, 0.0f};
            cubeTransform.isDirty = true;
            scene->markTransformDirty(cube);

            auto cubeChild = scene->createEntity("CubeChild");
            auto& childRenderer = registry.emplace<scene::MeshRendererComponent>(cubeChild);
            if (auto* resources = Engine::get().getRenderResourceRegistry())
            {
                childRenderer.meshId = resources->registerMesh("E:/github/April2/content/model/cube.gltf.asset");
                if (childRenderer.materialId == scene::kInvalidRenderID)
                {
                    childRenderer.materialId = resources->getMeshMaterialId(childRenderer.meshId, 0);
                }
            }
            childRenderer.enabled = true;

            auto& childTransform = registry.get<scene::TransformComponent>(cubeChild);
            childTransform.localPosition = {2.5f, 0.5f, 0.0f};
            childTransform.localScale = {0.6f, 0.6f, 0.6f};
            childTransform.isDirty = true;
            scene->markTransformDirty(cubeChild);

            scene->setParent(cubeChild, cube);
        }
    }

    auto ViewportWindow::queueViewportResize(EditorContext& context, float2 const& size) -> void
    {
        auto width = static_cast<uint32_t>(size.x);
        auto height = static_cast<uint32_t>(size.y);
        if (width == 0 || height == 0)
        {
            return;
        }

        if (context.viewportSize.x == size.x && context.viewportSize.y == size.y)
        {
            return;
        }

        context.viewportSize = size;
        m_pendingViewportSize = size;
        m_hasPendingViewportResize = true;
    }

    auto ViewportWindow::applyViewportResize(EditorContext& context) -> void
    {
        if (!m_hasPendingViewportResize)
        {
            return;
        }

        auto const size = m_pendingViewportSize;
        m_hasPendingViewportResize = false;

        auto width = static_cast<uint32_t>(size.x);
        auto height = static_cast<uint32_t>(size.y);
        if (width == 0 || height == 0)
        {
            return;
        }

        Engine::get().setSceneViewportSize(width, height);

        if (m_camera)
        {
            m_camera->setViewportSize(width, height);
        }

        if (m_cameraEntity != scene::NullEntity)
        {
            auto* scene = Engine::get().getSceneGraph();
            if (scene)
            {
                auto& registry = scene->getRegistry();
                auto& cameraComponent = registry.get<scene::CameraComponent>(m_cameraEntity);
                cameraComponent.viewportWidth = width;
                cameraComponent.viewportHeight = height;
                cameraComponent.isDirty = true;
            }
        }
    }

    auto ViewportWindow::onUIRender(EditorContext& context) -> void
    {
        ensureScene(context);

        if (!open)
        {
            return;
        }

        ui::ScopedWindow window{title(), &open};
        if (!window)
        {
            return;
        }

        auto hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        auto focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        if (m_camera)
        {
            auto const inputActive = hovered || focused;
            m_camera->setInputEnabled(inputActive);
            m_camera->onUpdate(ImGui::GetIO().DeltaTime);

            if (m_cameraEntity != scene::NullEntity)
            {
                auto* scene = Engine::get().getSceneGraph();
                if (scene)
                {
                    auto& registry = scene->getRegistry();
                    auto& transform = registry.get<scene::TransformComponent>(m_cameraEntity);
                    auto const selected = context.selection.entity == m_cameraEntity;

                    if (!inputActive && selected)
                    {
                        m_camera->setPosition(transform.localPosition);
                        m_camera->setRotation(transform.localRotation.x, transform.localRotation.y);
                    }
                    else
                    {
                        auto const position = m_camera->getPosition();
                        auto const direction = m_camera->getDirection();

                        auto const yaw = std::atan2(direction.x, -direction.z);
                        auto const pitch = std::asin(std::clamp(direction.y, -0.99f, 0.99f));

                        transform.localPosition = position;
                        transform.localRotation = {pitch, yaw, 0.0f};
                        transform.isDirty = true;
                        scene->markTransformDirty(m_cameraEntity);
                    }
                }
            }
        }

        auto size = ImGui::GetContentRegionAvail();
        queueViewportResize(context, {size.x, size.y});

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
            ImGui::Text("Viewport: %.0f x %.0f", context.viewportSize.x, context.viewportSize.y);
        }

    }
}
