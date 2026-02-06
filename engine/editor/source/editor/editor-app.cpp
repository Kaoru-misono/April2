#include "editor-app.hpp"

#include "element/editor-hierarchy.hpp"
#include "element/editor-inspector.hpp"
#include "element/editor-menu.hpp"
#include "element/editor-viewport.hpp"

#include <editor/element/element-logger.hpp>
#include <editor/element/element-profiler.hpp>
#include <scene/ecs-core.hpp>

#include <utility>

namespace april::editor
{
    auto EditorApp::registerElement(ElementFactory factory) -> void
    {
        if (factory)
        {
            m_factories.push_back(std::move(factory));
        }
    }

    auto EditorApp::registerDefaultElements() -> void
    {
        if (m_defaultsRegistered)
        {
            return;
        }
        m_defaultsRegistered = true;

        registerElement([](EditorContext& context, EditorApp& app) {
            return core::make_ref<EditorMenuElement>(
                context,
                app.m_onExit,
                app.getLogger(),
                app.getProfiler());
        });

        registerElement([](EditorContext& context, EditorApp& /*app*/) {
            return core::make_ref<EditorHierarchyElement>(context);
        });

        registerElement([](EditorContext& context, EditorApp& /*app*/) {
            return core::make_ref<EditorInspectorElement>(context);
        });

        registerElement([](EditorContext& context, EditorApp& /*app*/) {
            return core::make_ref<EditorViewportElement>(context);
        });

        registerElement([](EditorContext& /*context*/, EditorApp& app) {
            return app.getLogger();
        });

        registerElement([](EditorContext& /*context*/, EditorApp& app) {
            return app.getProfiler();
        });
    }

    auto EditorApp::install(Engine& engine, EditorUiConfig config) -> void
    {
        ensureDefaultSelection();

        m_uiConfig = std::move(config);



        if (!m_defaultsRegistered && m_factories.empty())
        {
            registerDefaultElements();
        }

        engine.addHooks(EngineHooks{
            .onInit = [this, &engine]() {
                initShell(engine);
            },
            .onShutdown = [this]() {
                m_shell.terminate();
                m_shellInitialized = false;
            },
            .onUIRender = [this](graphics::CommandContext* context, core::ref<graphics::TextureView> const& target) {
                m_shell.renderFrame(context, target);
            }
        });
    }

    auto EditorApp::getLogger() -> core::ref<ElementLogger>
    {
        if (!m_logger)
        {
            m_logger = core::make_ref<ElementLogger>(true);
            m_logger->setMenuEnabled(false);
        }
        return m_logger;
    }

    auto EditorApp::getProfiler() -> core::ref<ElementProfiler>
    {
        if (!m_profiler)
        {
            m_profiler = core::make_ref<ElementProfiler>(false);
            m_profiler->setMenuEnabled(false);
        }
        return m_profiler;
    }

    auto EditorApp::initShell(Engine& engine) -> void
    {
        if (m_shellInitialized)
        {
            return;
        }

        auto backendDesc = ImGuiBackendDesc{};
        backendDesc.device = engine.getDevice();
        backendDesc.window = engine.getWindow();
        backendDesc.vSync = engine.getWindow() ? engine.getWindow()->isVSync() : true;
        backendDesc.enableViewports = m_uiConfig.enableViewports;
        backendDesc.imguiConfigFlags = m_uiConfig.imguiConfigFlags;
        backendDesc.iniFilename = m_uiConfig.iniFilename;

        auto shellDesc = EditorShellDesc{};
        shellDesc.backend = std::move(backendDesc);
        shellDesc.useMenubar = m_uiConfig.useMenubar;
        shellDesc.dockSetup = m_uiConfig.dockSetup;

        m_shell.init(shellDesc);

        for (auto const& factory : m_factories)
        {
            auto element = factory(m_context, *this);
            if (element)
            {
                m_shell.addElement(element);
            }
        }

        m_shellInitialized = true;
    }

    auto EditorApp::ensureDefaultSelection() -> void
    {
        m_context.selection.entity = scene::NullEntity;
    }
}
