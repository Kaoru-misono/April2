#pragma once

#include "editor-context.hpp"
#include <runtime/engine.hpp>
#include <ui/element.hpp>
#include <core/foundation/object.hpp>

#include <functional>
#include <vector>

namespace april::ui
{
    class ElementLogger;
    class ElementProfiler;
}

namespace april::editor
{
    class EditorApp final
    {
    public:
        using ElementFactory = std::function<core::ref<ui::IElement>(EditorContext&, EditorApp&)>;

        EditorApp() = default;

        auto setOnExit(std::function<void()> onExit) -> void { m_onExit = std::move(onExit); }
        auto registerElement(ElementFactory factory) -> void;
        auto registerDefaultElements() -> void;
        auto install(Engine& engine) -> void;

        auto getContext() -> EditorContext& { return m_context; }
        auto getLogger() -> core::ref<ui::ElementLogger>;
        auto getProfiler() -> core::ref<ui::ElementProfiler>;

    private:
        auto ensureDefaultSelection() -> void;

        EditorContext m_context{};
        std::function<void()> m_onExit{};
        std::vector<ElementFactory> m_factories{};
        bool m_defaultsRegistered{false};
        core::ref<ui::ElementLogger> m_logger{};
        core::ref<ui::ElementProfiler> m_profiler{};
    };
}
