#include "element-profiler.hpp"

#include <core/profile/profile-manager.hpp>
#include <imgui.h>

namespace april::ui
{
    ElementProfiler::ElementProfiler(bool show)
        : m_show(show)
    {
    }

    auto ElementProfiler::onAttach(ImGuiLayer* pLayer) -> void
    {
    }

    auto ElementProfiler::onDetach() -> void
    {
    }

    auto ElementProfiler::onResize(graphics::CommandContext* pContext, float2 const& size) -> void
    {
    }

    auto ElementProfiler::onPreRender() -> void
    {
    }

    auto ElementProfiler::onRender(graphics::CommandContext* pContext) -> void
    {
    }

    auto ElementProfiler::onFileDrop(std::filesystem::path const& filename) -> void
    {
    }

    auto ElementProfiler::onUIMenu() -> void
    {
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Profiler", nullptr, &m_show);
            ImGui::EndMenu();
        }
    }

    auto ElementProfiler::onUIRender() -> void
    {
        if (!m_show)
        {
            return;
        }

        if (!m_paused)
        {
            auto events = april::core::ProfileManager::get().flush();
            auto const& threadNames = april::core::ProfileManager::get().getThreadNames();
            m_aggregator.ingest(events, threadNames);
            m_frames = m_aggregator.getFrames();
        }

        draw();
    }

    auto ElementProfiler::draw() -> void
    {
        if (!ImGui::Begin("Profiler", &m_show))
        {
            ImGui::End();
            return;
        }

        m_seenThisFrame.clear();

        if (ImGui::Button(m_paused ? "Resume" : "Pause"))
        {
            m_paused = !m_paused;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Stats"))
        {
            m_aggregator.clear();
            m_frames.clear();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Average", &m_showAvg);
        ImGui::SameLine();
        ImGui::Checkbox("Min/Max", &m_showMinMax);
        ImGui::SameLine();
        m_filter.Draw("Filter", 180.0f);

        ImGui::Separator();

        for (auto& frame : m_frames)
        {
            drawThread(frame);
        }

        ImGui::End();

        m_seenLastFrame.swap(m_seenThisFrame);
    }

    auto ElementProfiler::drawThread(april::core::ProfileThreadFrame& frame) -> void
    {
        auto label = frame.threadName.empty()
            ? std::string("Thread ") + std::to_string(frame.threadId)
            : frame.threadName;

        if (!ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            return;
        }

        ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
                                     ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg;

        int columnCount = 2 + (m_showAvg ? 1 : 0) + (m_showMinMax ? 2 : 0);
        auto tableId = std::string("ProfilerTable##") + std::to_string(frame.threadId);
        if (ImGui::BeginTable(tableId.c_str(), columnCount, tableFlags))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Last (ms)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            if (m_showAvg)
            {
                ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            }
            if (m_showMinMax)
            {
                ImGui::TableSetupColumn("Min (ms)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                ImGui::TableSetupColumn("Max (ms)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            }
            ImGui::TableHeadersRow();

        auto pathRoot = "t:" + std::to_string(frame.threadId);
        std::unordered_map<std::string, int> rootCounts;
        for (auto& node : frame.roots)
        {
            int& count = rootCounts[node.name];
            count += 1;
            auto path = pathRoot + "/" + node.name + "#" + std::to_string(count);
            drawNode(node, path);
        }

            ImGui::EndTable();
        }
    }

    auto ElementProfiler::nodeMatchesFilter(april::core::ProfileNode const& node) const -> bool
    {
        if (!m_filter.IsActive())
        {
            return true;
        }

        if (m_filter.PassFilter(node.name.c_str()))
        {
            return true;
        }

        for (auto const& child : node.children)
        {
            if (nodeMatchesFilter(child))
            {
                return true;
            }
        }

        return false;
    }

    auto ElementProfiler::drawNode(april::core::ProfileNode& node, std::string const& path) -> bool
    {
        if (!nodeMatchesFilter(node))
        {
            return false;
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;
        const bool hasChildren = !node.children.empty();
        if (!hasChildren)
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        ImGui::PushID(path.c_str());
        bool seenLast = m_seenLastFrame.contains(path);
        m_seenThisFrame.insert(path);
        if (hasChildren && !seenLast)
        {
            ImGui::SetNextItemOpen(getOpenState(path), ImGuiCond_Always);
        }
        bool open = ImGui::TreeNodeEx(node.name.c_str(), flags);
        if (hasChildren)
        {
            setOpenState(path, open);
        }

        auto drawValue = [](double us) {
            if (us <= 0.0)
            {
                ImGui::TextDisabled("--");
            }
            else
            {
                ImGui::Text("%.3f", us / 1000.0);
            }
        };

        ImGui::TableNextColumn();
        drawValue(node.lastUs);
        if (m_showAvg)
        {
            ImGui::TableNextColumn();
            drawValue(node.avgUs);
        }
        if (m_showMinMax)
        {
            ImGui::TableNextColumn();
            drawValue(node.minUs);
            ImGui::TableNextColumn();
            drawValue(node.maxUs);
        }

        if (open && !node.children.empty())
        {
            std::unordered_map<std::string, int> childCounts;
            for (auto& child : node.children)
            {
                int& count = childCounts[child.name];
                count += 1;
                auto childPath = path + "/" + child.name + "#" + std::to_string(count);
                drawNode(child, childPath);
            }
            ImGui::TreePop();
        }
        ImGui::PopID();

        return true;
    }

    auto ElementProfiler::getOpenState(std::string const& path) const -> bool
    {
        auto it = m_openState.find(path);
        if (it == m_openState.end())
        {
            return false;
        }
        return it->second;
    }

    auto ElementProfiler::setOpenState(std::string const& path, bool open) -> void
    {
        m_openState[path] = open;
    }
}
