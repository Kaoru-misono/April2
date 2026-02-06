#pragma once

#include "../editor-context.hpp"
#include <editor/editor-element.hpp>
#include <graphics/camera/simple-camera.hpp>
#include <scene/ecs-core.hpp>

#include <memory>

namespace april::editor
{
    class EditorViewportElement final : public IEditorElement
    {
        APRIL_OBJECT(EditorViewportElement)
    public:
        explicit EditorViewportElement(EditorContext& context)
            : m_context(context)
        {
        }

        auto onAttach(ImGuiBackend* pBackend) -> void override;
        auto onDetach() -> void override;
        auto onResize(graphics::CommandContext* pContext, float2 const& size) -> void override;
        auto onUIRender() -> void override;
        auto onUIMenu() -> void override;
        auto onPreRender() -> void override;
        auto onRender(graphics::CommandContext* pContext) -> void override;
        auto onFileDrop(std::filesystem::path const& filename) -> void override;

    private:
        EditorContext& m_context;
        std::unique_ptr<SimpleCamera> m_camera{};
        scene::Entity m_cameraEntity{scene::NullEntity};
    };
}
