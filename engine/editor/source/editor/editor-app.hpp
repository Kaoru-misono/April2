#pragma once

#include "editor-context.hpp"
#include "imgui-backend.hpp"
#include "window-manager.hpp"
#include "window-registry.hpp"
#include "tool-window.hpp"
#include "ui/action-manager.hpp"
#include <runtime/engine.hpp>
#include <core/foundation/object.hpp>

#include <imgui.h>

#include <functional>
#include <string>
#include <string_view>
#include <array>
#include <memory>
#include <vector>

namespace april::editor
{
    class ViewportWindow;

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
        using WindowFactory = std::function<std::unique_ptr<ToolWindow>(EditorContext&, EditorApp&)>;

        EditorApp() = default;

        auto setOnExit(std::function<void()> onExit) -> void { m_onExit = std::move(onExit); }
        auto registerWindow(WindowFactory factory) -> void;
        auto registerDefaultWindows() -> void;
        auto install(Engine& engine, EditorUiConfig config = {}) -> void;

        auto getContext() -> EditorContext& { return m_context; }
        auto getWindows() -> WindowRegistry& { return m_windows; }

    private:
        auto buildMenu(EditorContext& context) -> void;
        auto registerActions() -> void;
        auto renderMenuActions(std::string_view menu) -> void;
        auto ensureDefaultSelection() -> void;
        auto initWindowManager(Engine& engine) -> void;

        EditorContext m_context{};
        WindowRegistry m_windows{};
        core::ref<ImGuiBackend> m_backend{};
        ui::ActionManager m_actionManager{};
        std::function<void()> m_onExit{};
        std::vector<WindowFactory> m_factories{};
        bool m_defaultsRegistered{false};
        bool m_actionsRegistered{false};
        WindowManager m_windowManager{};
        EditorUiConfig m_uiConfig{};
        bool m_windowManagerInitialized{false};
        ViewportWindow* m_viewportWindow{};
        std::array<char, 260> m_importBuffer{};
    };
}
