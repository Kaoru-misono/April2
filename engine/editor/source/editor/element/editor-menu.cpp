#include "editor-menu.hpp"

#include "element-logger.hpp"
#include "element-profiler.hpp"

#include <runtime/engine.hpp>
#include <asset/asset-manager.hpp>
#include <imgui.h>

#include <filesystem>

namespace april::editor
{
    auto EditorMenuElement::onAttach(ImGuiBackend* /*pBackend*/) -> void {}
    auto EditorMenuElement::onDetach() -> void {}
    auto EditorMenuElement::onResize(graphics::CommandContext* /*pContext*/, float2 const& /*size*/) -> void {}
    auto EditorMenuElement::onUIRender() -> void {}
    auto EditorMenuElement::onPreRender() -> void {}
    auto EditorMenuElement::onRender(graphics::CommandContext* /*pContext*/) -> void {}
    auto EditorMenuElement::onFileDrop(std::filesystem::path const& /*filename*/) -> void {}

    auto EditorMenuElement::onUIMenu() -> void
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::BeginMenu("Import Asset"))
            {
                ImGui::InputText("Source Path", m_importBuffer.data(), m_importBuffer.size());
                if (ImGui::Button("Import"))
                {
                    auto* assetManager = Engine::get().getAssetManager();
                    if (assetManager)
                    {
                        assetManager->importAsset(std::filesystem::path{m_importBuffer.data()});
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Exit"))
            {
                if (m_onExit)
                {
                    m_onExit();
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            bool showStats = m_context.tools.showStats;
            if (ImGui::MenuItem("Stats", nullptr, &showStats))
            {
                m_context.tools.showStats = showStats;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            bool showLog = m_logger ? m_logger->isVisible() : false;
            if (ImGui::MenuItem("Log", nullptr, &showLog))
            {
                if (m_logger)
                {
                    m_logger->setVisible(showLog);
                }
            }

            bool showProfiler = m_profiler ? m_profiler->isVisible() : false;
            if (ImGui::MenuItem("Profiler", nullptr, &showProfiler))
            {
                if (m_profiler)
                {
                    m_profiler->setVisible(showProfiler);
                }
            }

            ImGui::EndMenu();
        }
    }
}
