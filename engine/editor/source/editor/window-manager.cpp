#include "window-manager.hpp"

#include <imgui_internal.h>

namespace april::editor
{
    auto WindowManager::init(WindowManagerDesc const& desc) -> void
    {
        m_imguiConfigFlags = desc.imguiConfigFlags;
        m_dockSetup = desc.dockSetup;
    }

    auto WindowManager::terminate() -> void
    {
        m_dockSetup = {};
    }

    auto WindowManager::beginFrame() -> void
    {
        setupDock();
    }

    auto WindowManager::renderWindows(EditorContext& context, WindowRegistry& windows) -> void
    {
        for (auto& window : windows.windows())
        {
            if (window && window->open)
            {
                window->onUIRender(context);
            }
        }
    }

    auto WindowManager::endFrame() -> void
    {
    }

    auto WindowManager::setupDock() -> void
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
                auto leftID = ImGui::DockBuilderSplitNode(dockID, ImGuiDir_Left, 0.22f, nullptr, &dockID);
                auto rightID = ImGui::DockBuilderSplitNode(dockID, ImGuiDir_Right, 0.28f, nullptr, &dockID);
                auto bottomID = ImGui::DockBuilderSplitNode(dockID, ImGuiDir_Down, 0.25f, nullptr, &dockID);

                ImGui::DockBuilderDockWindow("Hierarchy", leftID);
                ImGui::DockBuilderDockWindow("Inspector", rightID);
                ImGui::DockBuilderDockWindow("Console", bottomID);
                ImGui::DockBuilderDockWindow("Content Browser", bottomID);
            }
        }
    }
}
