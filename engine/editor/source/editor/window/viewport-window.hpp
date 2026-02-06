#pragma once

#include <editor/editor-camera.hpp>
#include <editor/tool-window.hpp>
#include <scene/ecs-core.hpp>

#include <memory>

namespace april::editor
{
    class ViewportWindow final : public ToolWindow
    {
    public:
        ViewportWindow() = default;
        ~ViewportWindow() override;

        [[nodiscard]] auto title() const -> char const* override { return "Viewport"; }
        auto onUIRender(EditorContext& context) -> void override;
        auto applyViewportResize(EditorContext& context) -> void;

    private:
        auto ensureScene(EditorContext& context) -> void;
        auto queueViewportResize(EditorContext& context, float2 const& size) -> void;

        std::unique_ptr<EditorCamera> m_camera{};
        scene::Entity m_cameraEntity{scene::NullEntity};
        float2 m_pendingViewportSize{};
        bool m_hasPendingViewportResize{false};
    };
}
