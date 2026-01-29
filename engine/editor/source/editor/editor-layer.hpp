#pragma once

#include <ui/element.hpp>
#include <graphics/rhi/command-context.hpp>
#include <core/math/type.hpp>

#include <functional>

namespace april::editor
{
    class EditorLayer final : public ui::IElement
    {
        APRIL_OBJECT(EditorLayer)
    public:
        EditorLayer() = default;
        ~EditorLayer() override = default;

        auto setOnExit(std::function<void()> onExit) -> void { m_onExit = std::move(onExit); }

        auto onAttach(ui::ImGuiLayer* pLayer) -> void override;
        auto onDetach() -> void override;
        auto onResize(graphics::CommandContext* pContext, float2 const& size) -> void override;
        auto onUIRender() -> void override;
        auto onUIMenu() -> void override;
        auto onPreRender() -> void override;
        auto onRender(graphics::CommandContext* pContext) -> void override;
        auto onFileDrop(std::filesystem::path const& filename) -> void override;

    private:
        std::function<void()> m_onExit{};
        float2 m_viewportSize{};
    };
}
