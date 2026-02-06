#include "editor-shell.hpp"

#include <imgui_internal.h>

namespace april::editor
{
    auto EditorShell::init(EditorShellDesc const& desc) -> void
    {
        m_backend = core::make_ref<ImGuiBackend>();
        m_backend->init(desc.backend);

        m_window = desc.backend.window;
        m_imguiConfigFlags = desc.backend.imguiConfigFlags;
        m_useMenubar = desc.useMenubar;
        m_dockSetup = desc.dockSetup;
    }

    auto EditorShell::terminate() -> void
    {
        for (auto& element : m_elements)
        {
            element->onDetach();
        }
        m_elements.clear();

        if (m_backend)
        {
            m_backend->terminate();
            m_backend.reset();
        }
    }

    auto EditorShell::addElement(core::ref<IEditorElement> const& element) -> void
    {
        if (!element)
        {
            return;
        }

        m_elements.emplace_back(element)->onAttach(m_backend.get());
    }

    auto EditorShell::renderFrame(graphics::CommandContext* pContext, core::ref<graphics::TextureView> const& pTarget) -> void
    {
        if (!m_backend)
        {
            return;
        }

        m_backend->newFrame();

        setupDock();

        if (m_useMenubar && ImGui::BeginMainMenuBar())
        {
            for (auto& element : m_elements)
            {
                element->onUIMenu();
            }
            ImGui::EndMainMenuBar();
        }

        for (auto& element : m_elements)
        {
            element->onUIRender();
        }

        updateViewportSize(pContext);

        ImGui::Render();

        for (auto& element : m_elements)
        {
            element->onPreRender();
        }

        for (auto& element : m_elements)
        {
            element->onRender(pContext);
        }

        m_backend->render(pContext, pTarget);
    }

    auto EditorShell::setupDock() -> void
    {
        if ((m_imguiConfigFlags & ImGuiConfigFlags_DockingEnable) == 0)
        {
            return;
        }

        auto const dockFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode;
        auto dockID = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockFlags);
        if (!ImGui::DockBuilderGetNode(dockID)->IsSplitNode() && !ImGui::FindWindowByName("Viewport"))
        {
            ImGui::DockBuilderDockWindow("Viewport", dockID);
            ImGui::DockBuilderGetCentralNode(dockID)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
            if (m_dockSetup)
            {
                m_dockSetup(dockID);
            }
            else
            {
                auto leftID = ImGui::DockBuilderSplitNode(dockID, ImGuiDir_Left, 0.2f, nullptr, &dockID);
                ImGui::DockBuilderDockWindow("Settings", leftID);
            }
        }
    }

    auto EditorShell::onViewportSizeChange(graphics::CommandContext* pContext, float2 size) -> void
    {
        if (m_window)
        {
            auto scale = m_window->getWindowContentScale();
            m_backend->setDpiScale(scale.x);
        }

        m_viewportSize = size;

        for (auto& element : m_elements)
        {
            element->onResize(pContext, m_viewportSize);
        }
    }

    auto EditorShell::updateViewportSize(graphics::CommandContext* pContext) -> void
    {
        auto viewportSize = float2{};
        if (auto const viewport = ImGui::FindWindowByName("Viewport"); viewport)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("Viewport");
            auto avail = ImGui::GetContentRegionAvail();
            viewportSize = {uint32_t(avail.x), uint32_t(avail.y)};
            ImGui::End();
            ImGui::PopStyleVar();
        }

        if (m_viewportSize.x != viewportSize.x || m_viewportSize.y != viewportSize.y)
        {
            onViewportSizeChange(pContext, viewportSize);
        }
    }
}
