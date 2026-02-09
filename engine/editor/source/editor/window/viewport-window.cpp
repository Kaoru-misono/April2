#include "viewport-window.hpp"

#include <editor/editor-context.hpp>
#include <editor/ui/ui.hpp>
#include <runtime/engine.hpp>
#include <scene/scene.hpp>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <algorithm>

namespace april::editor
{
    namespace
    {
        auto buildRotationMatrixFromEulerXyz(float3 const& euler) -> float4x4
        {
            auto matrix = float4x4{1.0f};
            matrix = glm::rotate(matrix, euler.x, float3{1.0f, 0.0f, 0.0f});
            matrix = glm::rotate(matrix, euler.y, float3{0.0f, 1.0f, 0.0f});
            matrix = glm::rotate(matrix, euler.z, float3{0.0f, 0.0f, 1.0f});
            return matrix;
        }

        auto buildRotationMatrixFromCameraBasis(float3 const& right, float3 const& up, float3 const& forward) -> float4x4
        {
            auto matrix = float4x4{1.0f};
            matrix[0] = float4{right, 0.0f};
            matrix[1] = float4{up, 0.0f};
            matrix[2] = float4{-forward, 0.0f};
            return matrix;
        }

        auto extractPitchYawFromForward(float3 const& forward) -> float2
        {
            auto const pitch = std::asin(std::clamp(forward.y, -0.99f, 0.99f));
            auto const yaw = std::atan2(forward.x, -forward.z);
            return {pitch, yaw};
        }
    }

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
                        auto const rotationMatrix = buildRotationMatrixFromEulerXyz(transform.localRotation);
                        auto const forward = glm::normalize(float3(rotationMatrix * float4{0.0f, 0.0f, -1.0f, 0.0f}));
                        auto const pitchYaw = extractPitchYawFromForward(forward);

                        m_camera->setPosition(transform.localPosition);
                        m_camera->setRotation(pitchYaw.x, pitchYaw.y);
                    }
                    else
                    {
                        auto const position = m_camera->getPosition();
                        auto const direction = m_camera->getDirection();
                        auto const right = m_camera->getRight();
                        auto const up = m_camera->getUp();

                        auto const rotationMatrix = buildRotationMatrixFromCameraBasis(right, up, direction);
                        auto eulerX = 0.0f;
                        auto eulerY = 0.0f;
                        auto eulerZ = 0.0f;
                        glm::extractEulerAngleXYZ(rotationMatrix, eulerX, eulerY, eulerZ);

                        transform.localPosition = position;
                        transform.localRotation = {eulerX, eulerY, eulerZ};
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
