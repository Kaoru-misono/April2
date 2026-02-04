#pragma once

#include "../editor-context.hpp"
#include <ui/element.hpp>

#include <scene/ecs-core.hpp>

namespace april::editor
{
    class EditorHierarchyElement final : public ui::IElement
    {
        APRIL_OBJECT(EditorHierarchyElement)
    public:
        explicit EditorHierarchyElement(EditorContext& context)
            : m_context(context)
        {
        }

        auto onAttach(ui::ImGuiLayer* pLayer) -> void override;
        auto onDetach() -> void override;
        auto onResize(graphics::CommandContext* pContext, float2 const& size) -> void override;
        auto onUIRender() -> void override;
        auto onUIMenu() -> void override;
        auto onPreRender() -> void override;
        auto onRender(graphics::CommandContext* pContext) -> void override;
        auto onFileDrop(std::filesystem::path const& filename) -> void override;

    private:
        EditorContext& m_context;
    };
}
