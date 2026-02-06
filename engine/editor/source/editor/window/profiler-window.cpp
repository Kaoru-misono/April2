#include "profiler-window.hpp"

#include <editor/ui/ui.hpp>
#include <imgui.h>

namespace april::editor
{
    ProfilerWindow::ProfilerWindow(bool show)
    {
        open = show;
    }

    auto ProfilerWindow::onUIRender(EditorContext& /*context*/) -> void
    {
        if (!open)
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

    auto ProfilerWindow::draw() -> void
    {
        ui::ScopedWindow window{title(), &open};
        if (!window)
        {
            return;
        }

        m_seenThisFrame.clear();

        auto toolbar = ui::Toolbar{};
        if (toolbar.button(m_paused ? "Resume" : "Pause"))
        {
            m_paused = !m_paused;
        }
        if (toolbar.button("Reset Stats"))
        {
            m_aggregator.clear();
            m_frames.clear();
        }
        toolbar.checkbox("Average", &m_showAvg);
        toolbar.checkbox("Min/Max", &m_showMinMax);
        toolbar.textFilter(m_filter, 180.0f);

        ImGui::Separator();

        for (auto& frame : m_frames)
        {
            drawThread(frame);
        }

        m_seenLastFrame.swap(m_seenThisFrame);
    }

    auto ProfilerWindow::drawThread(april::core::ProfileThreadFrame& frame) -> void
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
        ui::ScopedTable table{tableId.c_str(), columnCount, tableFlags};
        if (table)
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
        }
    }

    auto ProfilerWindow::nodeMatchesFilter(april::core::ProfileNode const& node) const -> bool
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

    auto ProfilerWindow::drawNode(april::core::ProfileNode& node, std::string const& path) -> bool
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

        ui::ScopedID nodeScope{path.c_str()};
        bool seenLast = m_seenLastFrame.contains(path);
        m_seenThisFrame.insert(path);
        if (hasChildren && !seenLast)
        {
            ImGui::SetNextItemOpen(getOpenState(path), ImGuiCond_Always);
        }
        bool openNode = ImGui::TreeNodeEx(node.name.c_str(), flags);
        if (hasChildren)
        {
            setOpenState(path, openNode);
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

        if (openNode && !node.children.empty())
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
        return true;
    }

    auto ProfilerWindow::getOpenState(std::string const& path) const -> bool
    {
        auto it = m_openState.find(path);
        if (it == m_openState.end())
        {
            return false;
        }
        return it->second;
    }

    auto ProfilerWindow::setOpenState(std::string const& path, bool openState) -> void
    {
        m_openState[path] = openState;
    }
}
