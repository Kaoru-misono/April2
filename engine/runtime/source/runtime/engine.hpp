#pragma once

#include <core/foundation/object.hpp>
#include <core/window/window.hpp>
#include <core/math/type.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/swapchain.hpp>
#include <ui/imgui-layer.hpp>
#include <ui/element.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace april
{
    struct EngineConfig
    {
        WindowDesc window{};
        graphics::Device::Desc device{};
        bool enableUI{true};
        bool vSync{true};
        float4 clearColor{0.1f, 0.1f, 0.1f, 1.0f};
        ui::ImGuiLayerDesc imgui{};
    };

    struct EngineHooks
    {
        std::function<void()> onInit{};
        std::function<void()> onShutdown{};
        std::function<void(float)> onUpdate{};
        std::function<void(graphics::CommandContext*, graphics::RenderTargetView*)> onRender{};
        std::function<void()> onUI{};
    };

    class Engine final : public core::Object
    {
        APRIL_OBJECT(Engine)
    public:
        Engine(EngineConfig const& config = {}, EngineHooks hooks = {});
        ~Engine() override;

        static auto get() -> Engine&;

        auto run() -> int;
        auto stop() -> void;

        auto addElement(core::ref<ui::IElement> const& element) -> void;

        auto getWindow() const -> Window* { return m_window.get(); }
        auto getDevice() const -> core::ref<graphics::Device> { return m_device; }
        auto getSwapchain() const -> core::ref<graphics::Swapchain> { return m_swapchain; }
        auto getImGuiLayer() const -> ui::ImGuiLayer* { return m_imguiLayer.get(); }
        auto isRunning() const -> bool { return m_running; }

    private:
        auto init() -> void;
        auto shutdown() -> void;
        auto attachPendingElements() -> void;

        EngineConfig m_config{};
        EngineHooks m_hooks{};

        static Engine* s_instance;

        bool m_running{false};
        bool m_initialized{false};
        bool m_swapchainDirty{false};

        std::unique_ptr<Window> m_window{};
        core::ref<graphics::Device> m_device{};
        core::ref<graphics::Swapchain> m_swapchain{};
        graphics::CommandContext* m_context{};
        core::ref<ui::ImGuiLayer> m_imguiLayer{};

        std::vector<core::ref<ui::IElement>> m_pendingElements{};
    };
}
