#pragma once

#include "editor-element.hpp"
#include "imgui-backend.hpp"

#include <core/window/window.hpp>
#include <core/math/type.hpp>

#include <imgui.h>

#include <functional>
#include <string>
#include <vector>

namespace april::editor
{
    struct EditorShellDesc
    {
        ImGuiBackendDesc backend{};
        bool useMenubar{true};
        std::function<void(ImGuiID)> dockSetup{};
    };

    class EditorShell final : public core::Object
    {
        APRIL_OBJECT(EditorShell)
    public:
        EditorShell() = default;
        ~EditorShell() override = default;

        auto init(EditorShellDesc const& desc) -> void;
        auto terminate() -> void;

        auto renderFrame(graphics::CommandContext* pContext, core::ref<graphics::TextureView> const& pTarget) -> void;
        auto addElement(core::ref<IEditorElement> const& element) -> void;

        [[nodiscard]] auto getBackend() const -> ImGuiBackend* { return m_backend.get(); }
        [[nodiscard]] auto getViewportSize() const -> float2 const& { return m_viewportSize; }

    private:
        auto setupDock() -> void;
        auto onViewportSizeChange(graphics::CommandContext* pContext, float2 size) -> void;
        auto updateViewportSize(graphics::CommandContext* pContext) -> void;

    private:
        core::ref<ImGuiBackend> m_backend{};
        Window* m_window{};
        ImGuiConfigFlags m_imguiConfigFlags{};
        bool m_useMenubar{true};
        std::function<void(ImGuiID)> m_dockSetup{};
        float2 m_viewportSize{};

        std::vector<core::ref<IEditorElement>> m_elements{};
    };
}
