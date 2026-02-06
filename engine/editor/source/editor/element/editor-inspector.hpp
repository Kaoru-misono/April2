#pragma once

#include "../editor-context.hpp"
#include <editor/editor-element.hpp>

#include <scene/ecs-core.hpp>

#include <array>

namespace april::editor
{
    class EditorInspectorElement final : public IEditorElement
    {
        APRIL_OBJECT(EditorInspectorElement)
    public:
        explicit EditorInspectorElement(EditorContext& context)
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
        scene::Entity m_lastEntity{scene::NullEntity};
        std::array<char, 128> m_tagBuffer{};
        std::array<char, 256> m_meshBuffer{};
    };
}
