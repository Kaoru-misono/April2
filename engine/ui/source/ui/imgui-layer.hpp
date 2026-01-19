#pragma once

#include "settings-handler.hpp"
#include "element.hpp"
#include <core/window/window.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/program/program-variables.hpp>

#include <vector>
#include <imgui.h>

struct ImDrawData;

namespace april::ui
{
    struct ImGuiLayerDesc
    {
        core::ref<graphics::Device> device{};

        // Window
        Window* window{};
        bool    vSync{true};

        // UI
        bool                         useMenu{true};                 // Include a menubar
        bool                         hasUndockableViewport{false};  // Allow floating windows
        std::function<void(ImGuiID)> dockSetup;                     // Dock layout setup
        ImGuiConfigFlags             imguiConfigFlags{ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable};
    };

    class ImGuiLayer final : public core::Object
    {
        APRIL_OBJECT(ImGuiLayer)
    public:
        ImGuiLayer();
        ~ImGuiLayer() override;

        auto init(ImGuiLayerDesc const& desc) -> void;
        auto terminate() -> void;

        auto beginFrame() -> void;
        auto endFrame(graphics::CommandContext* pContext, core::ref<graphics::RenderTargetView> const& pTarget) -> void;

        auto addElement(core::ref<IElement> const& element) -> void;

        auto getFontTexture() const -> core::ref<graphics::Texture> { return m_fontTexture; }

    private:
        auto setupImguiDock() -> void;
        auto onViewportSizeChange(float2 size) -> void;
        auto renderDrawData(graphics::RenderPassEncoder* pEncoder, ImDrawData* pDrawData) -> void;
        auto renderFrame(graphics::CommandContext* pContext) -> void;

    private:
        Window* m_window;
        core::ref<graphics::Device> mp_device;
        core::ref<graphics::Texture> m_fontTexture;
        core::ref<graphics::Program> m_program;
        core::ref<graphics::ProgramVariables> m_vars;
        core::ref<graphics::GraphicsPipeline> m_pipeline;
        core::ref<graphics::Sampler> m_fontSampler;
        core::ref<graphics::VertexLayout> m_layout;

        struct FrameResources
        {
            core::ref<graphics::Buffer> vertexBuffer;
            core::ref<graphics::Buffer> indexBuffer;
            uint32_t vertexCount{0};
            uint32_t indexCount{0};
        };

        std::vector<FrameResources> m_frameResources;
        uint32_t m_frameIndex{0};
        float m_dpiScale{1.0f};

        bool m_vsync{true};
        bool m_useMenubar{true};
        std::function<void(ImGuiID)> m_dockSetup;  // Function to setup the docking
        ImGuiConfigFlags m_imguiConfigFlags{};
        std::filesystem::path m_iniFileName{};
        SettingsHandler m_settingsHandler{};
        float2 m_viewportSize{};

        std::vector<core::ref<IElement>> m_elements{};
    };
}
