#pragma once

#include "../editor-context.hpp"
#include <ui/element.hpp>
#include <core/foundation/object.hpp>

#include <functional>
#include <array>

namespace april::ui
{
    class ElementLogger;
    class ElementProfiler;
}

namespace april::editor
{
    class EditorMenuElement final : public ui::IElement
    {
        APRIL_OBJECT(EditorMenuElement)
    public:
        EditorMenuElement(
            EditorContext& context,
            std::function<void()> onExit,
            core::ref<ui::ElementLogger> logger,
            core::ref<ui::ElementProfiler> profiler)
            : m_context(context)
            , m_onExit(std::move(onExit))
            , m_logger(std::move(logger))
            , m_profiler(std::move(profiler))
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
        std::function<void()> m_onExit;
        core::ref<ui::ElementLogger> m_logger;
        core::ref<ui::ElementProfiler> m_profiler;
        std::array<char, 260> m_importBuffer{};
    };
}
