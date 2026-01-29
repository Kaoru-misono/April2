#include "editor-app.hpp"

#include "element/editor-hierarchy.hpp"
#include "element/editor-inspector.hpp"
#include "element/editor-menu.hpp"
#include "element/editor-viewport.hpp"

#include <editor/element/element-logger.hpp>
#include <editor/element/element-profiler.hpp>

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

    auto EditorApp::install(Engine& engine) -> void
    {
        ensureDefaultSelection();

        if (!m_defaultsRegistered && m_factories.empty())
        {
            registerDefaultElements();
        }

        for (auto const& factory : m_factories)
        {
            auto element = factory(m_context, *this);
            if (element)
            {
                engine.addElement(element);
            }
        }
    }

    auto EditorApp::getLogger() -> core::ref<ui::ElementLogger>
    {
        if (!m_logger)
        {
            m_logger = core::make_ref<ui::ElementLogger>(true);
            m_logger->setMenuEnabled(false);
        }
        return m_logger;
    }

    auto EditorApp::getProfiler() -> core::ref<ui::ElementProfiler>
    {
        if (!m_profiler)
        {
            m_profiler = core::make_ref<ui::ElementProfiler>(true);
            m_profiler->setMenuEnabled(false);
        }
        return m_profiler;
    }

    auto EditorApp::ensureDefaultSelection() -> void
    {
        if (m_context.selection.entityName.empty())
        {
            m_context.selection.entityName = m_context.scene.name;
        }
    }
}
