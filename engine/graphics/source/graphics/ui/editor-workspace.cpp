#include "editor-workspace.hpp"
#include <imgui.h>
#include <imgui_internal.h>

namespace april::graphics
{
    EditorWorkspace::EditorWorkspace()
    {
    }

    EditorWorkspace::~EditorWorkspace()
    {
        Log::getLogger()->removeSink(m_logSinkId);
    }

    auto EditorWorkspace::init() -> void
    {
        m_logSinkId = Log::getLogger()->addSink([this](ELogLevel level, std::string const& prefix, std::string const& message) {
            m_logMessages.push_back({level, prefix, message});
            if (m_logMessages.size() > 1000)
            {
                m_logMessages.erase(m_logMessages.begin());
            }
        });
    }

    auto EditorWorkspace::addPanel(core::ref<EditorPanel> pPanel) -> void
    {
        m_panels.push_back(pPanel);
    }

    auto EditorWorkspace::onUIRender() -> void
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit", "Alt+F4")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                for (auto& pPanel : m_panels)
                {
                    bool open = pPanel->isOpen();
                    if (ImGui::MenuItem(pPanel->getName().c_str(), nullptr, &open))
                    {
                        pPanel->setOpen(open);
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help"))
            {
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        setupDockspace();

        for (auto& pPanel : m_panels)
        {
            if (pPanel->isOpen())
            {
                pPanel->onUIRender();
            }
        }

        renderViewport();
        renderHierarchy();
        renderInspector();
        renderContentBrowser();
        renderConsole();
    }

    auto EditorWorkspace::renderHierarchy() -> void
    {
        if (ImGui::Begin("Hierarchy"))
        {
            if (ImGui::TreeNode("Main Scene"))
            {
                if (ImGui::Selectable("Main Camera", false)) {}
                if (ImGui::Selectable("Directional Light", false)) {}
                if (ImGui::TreeNode("Static Mesh"))
                {
                    if (ImGui::Selectable("Triangle", true)) {}
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
        ImGui::End();
    }

    auto EditorWorkspace::renderInspector() -> void
    {
        if (ImGui::Begin("Inspector"))
        {
            ImGui::Text("Selected Object: Triangle");
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                static float pos[3] = { 0.0f, 0.0f, 0.0f };
                static float rot[3] = { 0.0f, 0.0f, 0.0f };
                static float sca[3] = { 1.0f, 1.0f, 1.0f };
                ImGui::DragFloat3("Position", pos, 0.1f);
                ImGui::DragFloat3("Rotation", rot, 0.1f);
                ImGui::DragFloat3("Scale", sca, 0.1f);
            }
            if (ImGui::CollapsingHeader("Mesh Renderer"))
            {
                ImGui::Text("Mesh: Triangle.obj");
                static float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
                ImGui::ColorEdit4("Base Color", color);
            }
        }
        ImGui::End();
    }

    auto EditorWorkspace::renderViewport() -> void
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin("Viewport"))
        {
            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
            
            if (mp_viewportSRV)
            {
                ImGui::Image((ImTextureID)mp_viewportSRV.get(), viewportPanelSize);
            }
            else
            {
                ImGui::Text("No Viewport Texture Set");
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    auto EditorWorkspace::renderContentBrowser() -> void
    {
        if (ImGui::Begin("Content Browser"))
        {
            if (m_currentContentPath != ".")
            {
                if (ImGui::Button(".."))
                {
                    m_currentContentPath = m_currentContentPath.parent_path();
                }
                ImGui::SameLine();
            }
            ImGui::Text("Path: %s", m_currentContentPath.string().c_str());
            ImGui::Separator();

            try
            {
                for (auto const& entry : std::filesystem::directory_iterator(m_currentContentPath))
                {
                    auto const& path = entry.path();
                    auto filename = path.filename().string();

                    if (entry.is_directory())
                    {
                        if (ImGui::Selectable((filename + "/").c_str()))
                        {
                            m_currentContentPath /= filename;
                        }
                    }
                    else
                    {
                        ImGui::Text("%s", filename.c_str());
                    }
                }
            }
            catch (std::exception const& e)
            {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", e.what());
            }
        }
        ImGui::End();
    }

    auto EditorWorkspace::renderConsole() -> void
    {
        if (ImGui::Begin("Console"))
        {
            if (ImGui::Button("Clear"))
            {
                m_logMessages.clear();
            }
            ImGui::Separator();

            if (ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
            {
                for (auto const& msg : m_logMessages)
                {
                    ImVec4 color = ImVec4(1, 1, 1, 1);
                    switch (msg.level)
                    {
                        case ELogLevel::Trace:    color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); break;
                        case ELogLevel::Info:     color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); break;
                        case ELogLevel::Warn:     color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break;
                        case ELogLevel::Error:    color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;
                        case ELogLevel::Critical: color = ImVec4(1.0f, 0.0f, 1.0f, 1.0f); break;
                    }
                    ImGui::TextColored(color, "%s%s", msg.prefix.c_str(), msg.message.c_str());
                }

                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                {
                    ImGui::SetScrollHereY(1.0f);
                }
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }

    auto EditorWorkspace::setupDockspace() -> void
    {
        const ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode;
        m_dockID = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockFlags);

        if (m_firstFrame)
        {
            m_firstFrame = false;

            if (!ImGui::DockBuilderGetNode(m_dockID)->IsSplitNode() && !ImGui::FindWindowByName("Viewport"))
            {
                ImGui::DockBuilderRemoveNode(m_dockID);
                ImGui::DockBuilderAddNode(m_dockID, dockFlags | ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(m_dockID, ImGui::GetMainViewport()->Size);

                ImGuiID viewportID = m_dockID;
                ImGuiID leftID = ImGui::DockBuilderSplitNode(viewportID, ImGuiDir_Left, 0.2f, nullptr, &viewportID);
                ImGuiID bottomID = ImGui::DockBuilderSplitNode(viewportID, ImGuiDir_Down, 0.3f, nullptr, &viewportID);
                ImGuiID rightID = ImGui::DockBuilderSplitNode(viewportID, ImGuiDir_Right, 0.25f, nullptr, &viewportID);

                ImGui::DockBuilderDockWindow("Viewport", viewportID);
                ImGui::DockBuilderDockWindow("Hierarchy", leftID);
                ImGui::DockBuilderDockWindow("Console", bottomID);
                ImGui::DockBuilderDockWindow("Content Browser", bottomID);
                ImGui::DockBuilderDockWindow("Inspector", rightID);

                ImGui::DockBuilderFinish(m_dockID);
            }
        }
    }
}
