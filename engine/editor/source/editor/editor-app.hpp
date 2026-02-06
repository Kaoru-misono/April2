#pragma once

#include "editor-context.hpp"
#include "editor-shell.hpp"
#include <runtime/engine.hpp>
#include <core/foundation/object.hpp>

#include <imgui.h>

#include <functional>
#include <string>
#include <vector>

namespace april::editor
{
    class ElementLogger;
    class ElementProfiler;

    struct EditorUiConfig
    {
        bool useMenubar{true};
        bool enableViewports{false};
        ImGuiConfigFlags imguiConfigFlags{ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable};
        std::string iniFilename{};
        std::function<void(ImGuiID)> dockSetup{};
    };

    class EditorApp final
    {
    public:
        using ElementFactory = std::function<core::ref<IEditorElement>(EditorContext&, EditorApp&)>;

        EditorApp() = default;

        auto setOnExit(std::function<void()> onExit) -> void { m_onExit = std::move(onExit); }
        auto registerElement(ElementFactory factory) -> void;
        auto registerDefaultElements() -> void;
        auto install(Engine& engine, EditorUiConfig config = {}) -> void;

        auto getContext() -> EditorContext& { return m_context; }
        auto getLogger() -> core::ref<ElementLogger>;
        auto getProfiler() -> core::ref<ElementProfiler>;

    private:
        auto ensureDefaultSelection() -> void;
        auto initShell(Engine& engine) -> void;

        EditorContext m_context{};
        std::function<void()> m_onExit{};
        std::vector<ElementFactory> m_factories{};
        bool m_defaultsRegistered{false};
        core::ref<ElementLogger> m_logger{};
        core::ref<ElementProfiler> m_profiler{};
        EditorShell m_shell{};
        EditorUiConfig m_uiConfig{};
        bool m_shellInitialized{false};
    };
}
