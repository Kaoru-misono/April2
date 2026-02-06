#pragma once

#include <ui/settings-handler.hpp>

#include <core/foundation/object.hpp>
#include <core/window/window.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/resource-views.hpp>
#include <graphics/program/program-variables.hpp>

#include <imgui.h>

#include <functional>
#include <string>
#include <vector>

struct ImDrawData;

namespace april::editor
{
    struct ImGuiBackendDesc
    {
        core::ref<graphics::Device> device{};

        Window* window{};
        bool vSync{true};

        bool enableViewports{false};
        std::string iniFilename{};
        ImGuiConfigFlags imguiConfigFlags{ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable};
    };

    class ImGuiBackend final : public core::Object
    {
        APRIL_OBJECT(ImGuiBackend)
    public:
        ImGuiBackend();
        ~ImGuiBackend() override;

        auto init(ImGuiBackendDesc const& desc) -> void;
        auto terminate() -> void;

        auto newFrame() -> void;
        auto render(graphics::CommandContext* pContext, core::ref<graphics::TextureView> const& pTarget) -> void;

        auto setIniFilename(std::string iniFilename) -> void;
        auto setDpiScale(float scale) -> void;

        auto getFontTexture() const -> core::ref<graphics::Texture> { return m_fontTexture; }

    private:
        auto renderDrawData(graphics::RenderPassEncoder* pEncoder, ImDrawData* pDrawData) -> void;

    private:
        Window* m_window{};
        core::ref<graphics::Device> mp_device{};
        core::ref<graphics::Texture> m_fontTexture{};
        core::ref<graphics::Program> m_program{};
        core::ref<graphics::ProgramVariables> m_vars{};
        core::ref<graphics::GraphicsPipeline> m_pipeline{};
        core::ref<graphics::Sampler> m_fontSampler{};
        core::ref<graphics::VertexLayout> m_layout{};

        struct FrameResources
        {
            core::ref<graphics::Buffer> vertexBuffer;
            core::ref<graphics::Buffer> indexBuffer;
            uint32_t vertexCount{0};
            uint32_t indexCount{0};
        };

        std::vector<FrameResources> m_frameResources{};
        uint32_t m_frameIndex{0};
        float m_dpiScale{1.0f};

        bool m_vsync{true};
        bool m_viewportsEnabled{false};
        ImGuiConfigFlags m_imguiConfigFlags{};
        std::string m_iniFileName{};
        ui::SettingsHandler m_settingsHandler{};
    };
}
