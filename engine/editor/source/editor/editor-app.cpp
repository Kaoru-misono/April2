#include "editor-app.hpp"

#include "window/console-window.hpp"
#include "window/content-browser-window.hpp"
#include "window/hierarchy-window.hpp"
#include "window/inspector-window.hpp"
#include "window/profiler-window.hpp"
#include "window/viewport-window.hpp"

#include <editor/ui/ui.hpp>
#include <asset/asset-manager.hpp>
#include <core/file/vfs.hpp>
#include <scene/ecs-core.hpp>
#include <imgui.h>

#include <filesystem>
#include <memory>
#include <utility>

namespace april::editor
{
    namespace
    {
        auto findProjectRoot(std::filesystem::path startPath) -> std::filesystem::path
        {
            auto current = std::filesystem::absolute(startPath);
            auto last = std::filesystem::path{};

            while (!current.empty() && current != last)
            {
                if (std::filesystem::exists(current / "CMakeLists.txt") ||
                    std::filesystem::exists(current / ".git"))
                {
                    return current;
                }

                last = current;
                current = current.parent_path();
            }

            return std::filesystem::absolute(startPath);
        }
    }

    auto EditorApp::registerWindow(WindowFactory factory) -> void
    {
        if (factory)
        {
            m_factories.push_back(std::move(factory));
        }
    }

    auto EditorApp::registerDefaultWindows() -> void
    {
        if (m_defaultsRegistered)
        {
            return;
        }
        m_defaultsRegistered = true;

        registerWindow([](EditorContext&, EditorApp& app) {
            auto window = std::make_unique<ViewportWindow>();
            app.m_viewportWindow = window.get();
            return window;
        });

        registerWindow([](EditorContext&, EditorApp&) {
            return std::make_unique<HierarchyWindow>();
        });

        registerWindow([](EditorContext&, EditorApp&) {
            return std::make_unique<InspectorWindow>();
        });

        registerWindow([](EditorContext&, EditorApp&) {
            return std::make_unique<ContentBrowserWindow>();
        });

        registerWindow([](EditorContext&, EditorApp&) {
            return std::make_unique<ConsoleWindow>(true);
        });

        registerWindow([](EditorContext&, EditorApp&) {
            return std::make_unique<ProfilerWindow>(false);
        });
    }

    auto EditorApp::install(Engine& engine, EditorUiConfig config) -> void
    {
        ensureDefaultSelection();

        m_uiConfig = std::move(config);
        m_context.assetManager = engine.getAssetManager();

        if (!m_defaultsRegistered && m_factories.empty() && m_windows.windows().empty())
        {
            registerDefaultWindows();
        }

        engine.addHooks(EngineHooks{
            .onInit = [this, &engine]() {
                initWindowManager(engine);
            },
            .onShutdown = [this]() {
                if (m_backend)
                {
                    m_backend->terminate();
                    m_backend.reset();
                }
                m_windowManager.terminate();
                m_windowManagerInitialized = false;
            },
            .onUpdate = [this](float /*delta*/) {
                if (m_viewportWindow)
                {
                    m_viewportWindow->applyViewportResize(m_context);
                }
            },
            .onUIRender = [this](graphics::CommandContext* context, core::ref<graphics::TextureView> const& target) {
                if (!m_backend)
                {
                    return;
                }

                m_backend->newFrame();
                m_windowManager.beginFrame();
                m_actionManager.processShortcuts();
                if (m_uiConfig.useMenubar)
                {
                    ui::ScopedMainMenuBar menuBar{};
                    if (menuBar)
                    {
                        buildMenu(m_context);
                    }
                }
                m_windowManager.renderWindows(m_context, m_windows);
                m_windowManager.endFrame();
                ImGui::Render();
                m_backend->render(context, target);
            }
        });
    }

    auto EditorApp::initWindowManager(Engine& engine) -> void
    {
        if (m_windowManagerInitialized)
        {
            return;
        }

        m_context.assetManager = engine.getAssetManager();

        auto projectRoot = findProjectRoot(std::filesystem::current_path());
        april::VFS::init();
        april::VFS::mount("project", projectRoot);

        auto assetRoot = m_context.assetManager
            ? m_context.assetManager->getAssetRoot()
            : std::filesystem::path{"content"};

        if (assetRoot.is_relative())
        {
            auto assetPhysical = projectRoot / assetRoot;
            april::VFS::mount(assetRoot.generic_string(), assetPhysical);
        }

        auto backendDesc = ImGuiBackendDesc{};
        backendDesc.device = engine.getDevice();
        backendDesc.window = engine.getWindow();
        backendDesc.vSync = engine.getWindow() ? engine.getWindow()->isVSync() : true;
        backendDesc.enableViewports = m_uiConfig.enableViewports;
        backendDesc.imguiConfigFlags = m_uiConfig.imguiConfigFlags;
        backendDesc.iniFilename = m_uiConfig.iniFilename;

        m_backend = core::make_ref<ImGuiBackend>();
        m_backend->init(backendDesc);

        auto managerDesc = WindowManagerDesc{};
        managerDesc.imguiConfigFlags = m_uiConfig.imguiConfigFlags;
        managerDesc.dockSetup = m_uiConfig.dockSetup;

        m_windowManager.init(managerDesc);

        for (auto const& factory : m_factories)
        {
            auto window = factory(m_context, *this);
            if (window)
            {
                m_windows.add(std::move(window));
            }
        }

        registerActions();

        m_windowManagerInitialized = true;
    }

    auto EditorApp::ensureDefaultSelection() -> void
    {
        m_context.selection.entity = scene::NullEntity;
    }

    auto EditorApp::buildMenu(EditorContext& context) -> void
    {
        if (ui::ScopedMenu fileMenu{"File"})
        {
            if (ui::ScopedMenu importMenu{"Import Asset"})
            {
                ImGui::InputText("Source Path", m_importBuffer.data(), m_importBuffer.size());
                if (ImGui::Button("Import"))
                {
                    if (context.assetManager)
                    {
                        auto asset = context.assetManager->importAsset(std::filesystem::path{m_importBuffer.data()});
                        if (asset)
                        {
                            if (auto* window = m_windows.findByTitle("Content Browser"))
                            {
                                if (auto* contentBrowser = dynamic_cast<ContentBrowserWindow*>(window))
                                {
                                    contentBrowser->requestRefresh();
                                }
                            }
                        }
                    }
                }
            }

            ImGui::Separator();
            renderMenuActions("File");
        }

        if (ui::ScopedMenu editMenu{"Edit"})
        {
            renderMenuActions("Edit");
        }

        if (ui::ScopedMenu viewMenu{"View"})
        {
            renderMenuActions("View");
        }

        if (ui::ScopedMenu windowMenu{"Window"})
        {
            renderMenuActions("Window");
        }
    }

    auto EditorApp::registerActions() -> void
    {
        if (m_actionsRegistered)
        {
            return;
        }
        m_actionsRegistered = true;

        m_actionManager.registerAction("File", "Exit", "Alt+F4", [this]() {
            if (m_onExit)
            {
                m_onExit();
            }
        });

        m_actionManager.registerAction(
            "Edit",
            "Undo",
            "Ctrl+Z",
            [this]() { m_context.commandStack.undo(); },
            [this]() { return m_context.commandStack.canUndo(); }
        );

        m_actionManager.registerAction(
            "Edit",
            "Redo",
            "Ctrl+Shift+Z",
            [this]() { m_context.commandStack.redo(); },
            [this]() { return m_context.commandStack.canRedo(); }
        );

        m_actionManager.registerAction(
            "",
            "RedoAlt",
            "Ctrl+Y",
            [this]() { m_context.commandStack.redo(); },
            [this]() { return m_context.commandStack.canRedo(); }
        );

        m_actionManager.registerAction(
            "View",
            "Stats",
            "",
            [this]() { m_context.tools.showStats = !m_context.tools.showStats; },
            {},
            [this]() { return m_context.tools.showStats; }
        );

        for (auto const& window : m_windows.windows())
        {
            if (!window || !window->title())
            {
                continue;
            }

            auto title = std::string{window->title()};
            auto shortcut = std::string{};

            if (title == "Hierarchy") shortcut = "Ctrl+Shift+H";
            else if (title == "Inspector") shortcut = "Ctrl+Shift+I";
            else if (title == "Viewport") shortcut = "Ctrl+Shift+V";
            else if (title == "Content Browser") shortcut = "Ctrl+Shift+B";
            else if (title == "Console") shortcut = "Ctrl+Shift+C";
            else if (title == "Profiler") shortcut = "Ctrl+Shift+P";

            m_actionManager.registerAction(
                "Window",
                title,
                shortcut,
                [this, title]() {
                    if (auto* target = m_windows.findByTitle(title))
                    {
                        target->open = !target->open;
                    }
                },
                {},
                [this, title]() {
                    if (auto* target = m_windows.findByTitle(title))
                    {
                        return target->open;
                    }
                    return false;
                }
            );
        }
    }

    auto EditorApp::renderMenuActions(std::string_view menu) -> void
    {
        for (auto* action : m_actionManager.getMenuActions(menu))
        {
            if (!action)
            {
                continue;
            }

            auto const enabled = !action->isEnabled || action->isEnabled();
            auto const checked = action->isChecked && action->isChecked();
            auto const* shortcut = action->shortcut.empty() ? nullptr : action->shortcut.c_str();

            if (ImGui::MenuItem(action->name.c_str(), shortcut, checked, enabled))
            {
                if (action->callback)
                {
                    action->callback();
                }
            }
        }
    }
}
