#pragma once

#include "window-registry.hpp"
#include "editor-context.hpp"

#include <core/foundation/object.hpp>

#include <imgui.h>

#include <functional>

namespace april::editor
{
    struct WindowManagerDesc
    {
        ImGuiConfigFlags imguiConfigFlags{ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable};
        std::function<void(ImGuiID)> dockSetup{};
    };

    class WindowManager final : public core::Object
    {
        APRIL_OBJECT(WindowManager)
    public:
        WindowManager() = default;
        ~WindowManager() override = default;

        auto init(WindowManagerDesc const& desc) -> void;
        auto terminate() -> void;

        auto beginFrame() -> void;
        auto renderWindows(EditorContext& context, WindowRegistry& windows) -> void;
        auto endFrame() -> void;

    private:
        auto setupDock() -> void;

    private:
        ImGuiConfigFlags m_imguiConfigFlags{};
        std::function<void(ImGuiID)> m_dockSetup{};
    };
}
