#pragma once

#include <core/window/window.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/program/program-variables.hpp>
#include "imgui-backend.hpp"
#include "editor-workspace.hpp"

#include <vector>
#include <memory>
#include <imgui.h>

struct ImDrawData;

namespace april::graphics
{
    class ImGuiLayer final : public core::Object
    {
        APRIL_OBJECT(ImGuiLayer)
    public:
        ImGuiLayer(Window& window, core::ref<graphics::Device> pDevice);
        ~ImGuiLayer() override;

        auto begin() -> void;
        auto end(CommandContext* pContext, RenderTargetView* pTarget) -> void;

        auto setViewportTexture(core::ref<ShaderResourceView> pSRV) -> void;

        auto setFontScale(float scale) -> void { m_fontScale = scale; m_fontDirty = true; }
        [[nodiscard]] auto getFontScale() const -> float { return m_fontScale; }

        [[nodiscard]] auto isEditorMode() const -> bool { return m_editorMode; }

    private:
        auto initApi() -> void;

    private:
        Window& m_window;
        core::ref<graphics::Device> mp_device;
        std::unique_ptr<IImGuiBackend> mp_backend;
        std::unique_ptr<EditorWorkspace> mp_workspace;

        bool m_editorMode{true};
        float m_fontScale{1.0f};
        bool m_fontDirty{false};

        float m_lastDpiScale{0.0f};
        float m_lastFinalScale{0.0f};
        ImGuiStyle m_baseStyle;
    };
}