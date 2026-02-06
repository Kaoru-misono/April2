#pragma once

#include <core/foundation/object.hpp>
#include <core/window/window.hpp>
#include <core/math/type.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/render-target.hpp>
#include <graphics/rhi/resource-views.hpp>
#include <graphics/rhi/swapchain.hpp>
#include <graphics/rhi/texture.hpp>
#include <asset/asset-manager.hpp>
#include <scene/scene.hpp>

#include <functional>
#include <filesystem>
#include <memory>
#include <vector>

namespace april
{
    struct EngineConfig
    {
        WindowDesc window{};
        graphics::Device::Desc device{};
        bool vSync{true};
        bool compositeSceneToOutput{true};
        float4 clearColor{0.1f, 0.1f, 0.1f, 1.0f};
        std::filesystem::path assetRoot{"content"};
        std::filesystem::path ddcRoot{"build/cache/DDC"};
    };

    struct EngineHooks
    {
        std::function<void()> onInit{};
        std::function<void()> onShutdown{};
        std::function<void(float)> onUpdate{};
        std::function<void(graphics::CommandContext*, graphics::TextureView*)> onRender{};
        std::function<void(graphics::CommandContext*, core::ref<graphics::TextureView> const&)> onUIRender{};
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

        auto getWindow() const -> Window* { return m_window.get(); }
        auto getDevice() const -> core::ref<graphics::Device> { return m_device; }
        auto getSwapchain() const -> core::ref<graphics::Swapchain> { return m_swapchain; }
        auto getSceneColorSrv() const -> core::ref<graphics::TextureView>;
        auto setSceneViewportSize(uint32_t width, uint32_t height) -> void;
        auto isRunning() const -> bool { return m_running; }
        auto getSceneGraph() -> scene::SceneGraph* { return m_sceneGraph.get(); }
        auto getAssetManager() -> asset::AssetManager* { return m_assetManager.get(); }

        auto addHooks(EngineHooks hooks) -> void;

    private:
        auto init() -> void;
        auto renderFrame(float deltaTime) -> void;
        auto shutdown() -> void;
        auto ensureOffscreenTarget(uint32_t width, uint32_t height) -> void;

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
        std::unique_ptr<asset::AssetManager> m_assetManager{};
        std::unique_ptr<scene::SceneGraph> m_sceneGraph{};
        core::ref<scene::SceneRenderer> m_renderer{};
        core::ref<graphics::Texture> m_offscreen{};
        uint32_t m_offscreenWidth{0};
        uint32_t m_offscreenHeight{0};
    };
}
