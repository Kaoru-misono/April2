#include "editor-viewport.hpp"

#include <runtime/engine.hpp>
#include <scene/scene.hpp>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

namespace april::editor
{
    auto EditorViewportElement::onAttach(ui::ImGuiLayer* /*pLayer*/) -> void
    {
        auto constexpr kDefaultFov = 45.0f;
        m_camera = std::make_unique<SimpleCamera>(glm::radians(kDefaultFov), 1.777f, 0.1f, 1000.0f);
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

        auto const* meshPool = registry.getPool<scene::MeshRendererComponent>();
        if (!meshPool || meshPool->data().empty())
        {
            auto cube = scene->createEntity("Cube");
            auto& meshRenderer = registry.emplace<scene::MeshRendererComponent>(cube);
            meshRenderer.meshAssetPath = "E:/github/April2/content/model/cube.gltf.asset";
            meshRenderer.enabled = true;

            auto& cubeTransform = registry.get<scene::TransformComponent>(cube);
            cubeTransform.localPosition = {0.0f, 0.0f, 0.0f};
            cubeTransform.isDirty = true;

            auto cubeChild = scene->createEntity("CubeChild");
            auto& childRenderer = registry.emplace<scene::MeshRendererComponent>(cubeChild);
            childRenderer.meshAssetPath = "E:/github/April2/content/model/cube.gltf.asset";
            childRenderer.enabled = true;

            auto& childTransform = registry.get<scene::TransformComponent>(cubeChild);
            childTransform.localPosition = {2.5f, 0.5f, 0.0f};
            childTransform.localScale = {0.6f, 0.6f, 0.6f};
            childTransform.isDirty = true;

            scene->setParent(cubeChild, cube);
        }
    }

    auto EditorViewportElement::onDetach() -> void
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
    }

    auto EditorViewportElement::onUIRender() -> void
    {
        ImGui::Begin("Viewport");
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
                    auto const selected = m_context.selection.entity == m_cameraEntity;

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
                    }
                }
            }
        }
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
