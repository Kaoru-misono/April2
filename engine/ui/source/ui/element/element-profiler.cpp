#include "element-profiler.hpp"
#include "ui/font/fonts.hpp"
#include "ui/font/external/IconsMaterialSymbols.h"
#include <core/profile/profiler.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <implot.h>
#include <algorithm>
#include <vector>
#include <cstdio>

using namespace april::core;

namespace april::ui
{
    ElementProfiler::ElementProfiler(std::shared_ptr<ViewSettings> defaultSettings)
    {
        m_profiler = &Profiler::get();
        m_views.push_back({.state = defaultSettings ? defaultSettings : std::make_shared<ViewSettings>()});
    }

    auto ElementProfiler::onAttach(ImGuiLayer*) -> void
    {
        add_settings_handler();
    }

    auto ElementProfiler::onDetach() -> void {}
    auto ElementProfiler::onResize(graphics::CommandContext* pContext, float2 const& size) -> void {}
    auto ElementProfiler::onPreRender() -> void {}
    auto ElementProfiler::onRender(graphics::CommandContext* pContext) -> void {}
    auto ElementProfiler::onFileDrop(std::filesystem::path const& filename) -> void {}

    auto ElementProfiler::addView(std::shared_ptr<ViewSettings> state) -> void
    {
        m_views.push_back({.state = state});
    }

    auto ElementProfiler::add_settings_handler() -> void
    {
        auto readOpen = [](ImGuiContext*, ImGuiSettingsHandler* handler, const char* name) -> void* {
            auto* self = static_cast<ElementProfiler*>(handler->UserData);
            for(size_t i = 0; i < self->m_views.size(); ++i) {
                if(self->m_views[i].state->name == name) return (void*)(uintptr_t)(i + 1);
            }
            return nullptr;
        };

        auto saveAllToIni = [](ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
            auto* self = static_cast<ElementProfiler*>(handler->UserData);
            for(const auto& view : self->m_views) {
                buf->appendf("[%s][%s]\n", handler->TypeName, view.state->name.c_str());
                buf->appendf("ShowWindow=%d\n", view.state->show ? 1 : 0);
            }
            buf->append("\n");
        };

        auto loadLineFromIni = [](ImGuiContext*, ImGuiSettingsHandler* handler, void* entry, const char* line) {
            auto* self = static_cast<ElementProfiler*>(handler->UserData);
            intptr_t view_id = (intptr_t)entry - 1;
            int value;
            if(sscanf(line, "ShowWindow=%d", &value) == 1) {
                if (view_id >= 0 && view_id < (intptr_t)self->m_views.size()) {
                    self->m_views[view_id].state->show = (value != 0);
                }
            }
        };

        ImGuiSettingsHandler iniHandler;
        iniHandler.TypeName = "ElementProfiler";
        iniHandler.TypeHash = ImHashStr(iniHandler.TypeName);
        iniHandler.ReadOpenFn = readOpen;
        iniHandler.WriteAllFn = saveAllToIni;
        iniHandler.ReadLineFn = loadLineFromIni;
        iniHandler.UserData = this;
        ImGui::GetCurrentContext()->SettingsHandlers.push_back(iniHandler);
    }

    auto ElementProfiler::onUIMenu() -> void
    {
        if (ImGui::BeginMenu("View"))
        {
            for(auto& view : m_views)
            {
                ImGui::MenuItem((std::string(ICON_MS_BLOOD_PRESSURE " ") + view.state->name).c_str(), nullptr, &view.state->show);
            }
            ImGui::EndMenu();
        }
    }

    auto ElementProfiler::onUIRender() -> void
    {
        constexpr auto deltaTime = float{1.0f / 60.0f};
        static auto s_timeElapsed = float{0};
        s_timeElapsed += ImGui::GetIO().DeltaTime;

        bool showWindow = false;
        for(const auto& view : m_views) showWindow |= view.state->show;

        if (showWindow && s_timeElapsed >= deltaTime)
        {
            s_timeElapsed = 0;
            update_data();
        }

        for (auto& view : m_views)
        {
            if (!view.state->show) continue;

            if (ImGui::Begin(view.state->name.c_str(), &view.state->show))
            {
                if (ImGui::BeginTabBar("Profiler Tabs"))
                {
                    auto tabFlags = [&view](TabId id) { return view.selectDefaultTab && view.state->defaultTab == id ? ImGuiTabItemFlags_SetSelected : 0; };

                    if (ImGui::BeginTabItem("Table", nullptr, tabFlags(TabId::TABLE))) {
                        render_table(view);
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("BarChart", nullptr, tabFlags(TabId::BAR_CHART))) {
                        render_bar_chart(view);
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("LineChart", nullptr, tabFlags(TabId::LINE_CHART))) {
                        render_line_chart(view);
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("PieChart", nullptr, tabFlags(TabId::PIE_CHART))) {
                        render_pie_chart(view);
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                view.selectDefaultTab = false;
            }
            ImGui::End();
        }
    }

    auto ElementProfiler::update_data() -> void
    {
        if (!m_profiler) return;

        m_profiler->getSnapshots(m_frameSnapshots, m_singleSnapshots);

        m_frameNodes.clear();
        m_singleNodes.clear();

        for(auto const& snapshot : m_frameSnapshots)
        {
            auto entryNode = EntryNode{};
            entryNode.name = snapshot.name;
            m_frameNodes.emplace_back(entryNode);
            add_entries(snapshot, m_frameNodes.back().child, 0, (uint32_t)snapshot.timerInfos.size(), 0);
        }
        for(auto const& snapshot : m_singleSnapshots)
        {
            auto entryNode = EntryNode{};
            entryNode.name = snapshot.name;
            m_singleNodes.emplace_back(entryNode);
            add_entries(snapshot, m_singleNodes.back().child, 0, (uint32_t)snapshot.timerInfos.size(), 0);
        }
    }

    auto ElementProfiler::add_entries(
        april::core::Snapshot const& snapshot,
        std::vector<EntryNode>& nodes,
        uint32_t startIndex, uint32_t endIndex, uint32_t currentLevel
    ) -> uint32_t
    {
        for(auto curIndex = startIndex; curIndex < endIndex; curIndex++)
        {
            auto const& timerInfo = snapshot.timerInfos[curIndex];
            auto const& level = timerInfo.level;

            if(level < currentLevel) return curIndex;

            auto const& name = snapshot.timerNames[curIndex];

            auto entryNode = EntryNode{};
            entryNode.name = name.empty() ? "N/A" : name;
            entryNode.gpuTime = static_cast<float>(timerInfo.gpu.average / 1000.0);
            entryNode.cpuTime = static_cast<float>(timerInfo.cpu.average / 1000.0);
            entryNode.timerInfo = timerInfo;

            if(timerInfo.async)
            {
                nodes.emplace_back(entryNode);
                continue;
            }

            auto nextLevel = curIndex + 1 < endIndex ? snapshot.timerInfos[curIndex + 1].level : currentLevel;
            if(nextLevel > currentLevel)
            {
                curIndex = add_entries(snapshot, entryNode.child, curIndex + 1, endIndex, nextLevel);
            }
            nodes.emplace_back(entryNode);
            if(nextLevel < currentLevel) return curIndex;
        }
        return endIndex;
    }

    auto ElementProfiler::display_table_node(EntryNode const& node, bool detailed, uint32_t defaultOpenLevels, uint32_t depth) -> void
    {
        auto flags = ImGuiTableFlags{ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanAllColumns};
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        auto const is_folder = (node.child.empty() == false);
        flags = is_folder ? flags | (depth < defaultOpenLevels ? ImGuiTreeNodeFlags_DefaultOpen : 0) :
                            flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        auto open = ImGui::TreeNodeEx(node.name.c_str(), flags);
        auto const& info = node.timerInfo;

        auto drawValue = [](double value) {
            if(value <= 0) ImGui::TextDisabled("--");
            else ImGui::Text("%3.3f", value / 1000.0f);
        };

        ImGui::PushFont(april::ui::getMonospaceFont());
        ImGui::TableNextColumn(); drawValue(info.gpu.average);
        if(detailed) {
            ImGui::TableNextColumn(); drawValue(info.gpu.last);
            ImGui::TableNextColumn(); drawValue(info.gpu.absMinValue);
            ImGui::TableNextColumn(); drawValue(info.gpu.absMaxValue);
        }
        ImGui::TableNextColumn(); drawValue(info.cpu.average);
        if(detailed) {
            ImGui::TableNextColumn(); drawValue(info.cpu.last);
            ImGui::TableNextColumn(); drawValue(info.cpu.absMinValue);
            ImGui::TableNextColumn(); drawValue(info.cpu.absMaxValue);
        }
        ImGui::PopFont();

        if((open) && is_folder)
        {
            for(const auto& child : node.child)
            {
                display_table_node(child, detailed, defaultOpenLevels, depth + 1);
            }
            ImGui::TreePop();
        }
    }

    auto ElementProfiler::render_table(View& view) -> void
    {
        draw_vsync_checkbox();
        ImGui::SameLine();
        ImGui::Checkbox("Detailed", &view.state->table.detailed);

        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 38);
        auto copy = false;
        if(ImGui::Button(ICON_MS_CONTENT_COPY))
        {
            ImGui::LogToClipboard();
            copy = true;
        }
        if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Copy data to clipboard");

        // Grid Mode
        const int minGridSize = view.state->table.detailed ? 1500 : 550;
        const bool gridMode = ImGui::GetContentRegionAvail().x >= minGridSize && m_frameNodes.size() > 1;
        const float width   = ImGui::GetContentRegionAvail().x / (gridMode ? 2.0f : 1.0f) - 5.0f;

        const auto colCount = view.state->table.detailed ? 9 : 3;
        const auto tableFlags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;

        for(size_t i = 0; i < m_frameNodes.size(); ++i)
        {
            if (i > 0)
            {
                if (gridMode && i % 2 != 0) ImGui::SameLine();
                else ImGui::Spacing();
            }

            // Only render if there are children or single nodes
            if (!m_frameNodes[i].child.empty() || (i < m_singleNodes.size() && !m_singleNodes[i].child.empty()))
            {
                if (ImGui::BeginTable("EntryTable", colCount, tableFlags, ImVec2(width, 0)))
                {
                    ImGui::TableSetupColumn(m_frameNodes[i].name.c_str(), ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthFixed, 250.);
                    ImGui::TableSetupColumn("GPU avg", ImGuiTableColumnFlags_WidthStretch);
                    if(view.state->table.detailed) {
                        ImGui::TableSetupColumn("GPU last", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("GPU min", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("GPU max", ImGuiTableColumnFlags_WidthStretch);
                    }
                    ImGui::TableSetupColumn("CPU avg", ImGuiTableColumnFlags_WidthStretch);
                    if(view.state->table.detailed) {
                        ImGui::TableSetupColumn("CPU last", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("CPU min", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("CPU max", ImGuiTableColumnFlags_WidthStretch);
                    }
                    ImGui::TableHeadersRow();

                    for(const auto& child : m_frameNodes[i].child)
                        display_table_node(child, view.state->table.detailed, view.state->table.levels, 0);

                    if (i < m_singleNodes.size())
                    {
                        for(const auto& child : m_singleNodes[i].child)
                            display_table_node(child, view.state->table.detailed, view.state->table.levels, 0);
                    }

                    ImGui::EndTable();
                }
            }
        }

        if(copy) ImGui::LogFinish();
    }

    auto ElementProfiler::render_pie_chart(View& view) -> void
    {
        // Grid Mode Logic
        const bool gridMode = ImGui::GetContentRegionAvail().x >= 600 && m_frameNodes.size() > 1;
        const float width   = ImGui::GetContentRegionAvail().x / (gridMode ? 2.0f : 1.0f) - 5.0f;
        const float legendWidth = 170.0f;
        const float chartWidth  = (ImGui::GetContentRegionAvail().x - legendWidth) / (gridMode ? 2.0f : 1.0f) - 5.0f;

        draw_vsync_checkbox();
        ImGui::SameLine();
        ImGui::Checkbox("CPU total", &view.state->pieChart.cpuTotal);
        if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Use CPU frame time as total time, otherwise use sum of GPU timers.");

        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.F);
        ImGui::InputInt("Levels", &view.state->pieChart.levels);
        view.state->pieChart.levels = std::max(1, view.state->pieChart.levels);

        for(size_t i = 0; i < m_frameNodes.size(); ++i)
        {
            auto const& rootNode = m_frameNodes[i];
            if(!rootNode.child.empty())
            {
                auto const& node = rootNode.child[0];

                if(gridMode && i % 2 != 0) ImGui::SameLine();

                if(i % 3 == 0) ImPlot::PushColormap(ImPlotColormap_Deep);
                else if(i % 3 == 1) ImPlot::PushColormap(ImPlotColormap_Pastel);
                else ImPlot::PushColormap(ImPlotColormap_Viridis);

                if(ImPlot::BeginPlot(rootNode.name.c_str(), ImVec2(width, (float)view.state->plotHeight), ImPlotFlags_Equal | ImPlotFlags_NoMouseText))
                {
                    ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_Lock, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_Lock);
                    ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);

                    const float aspectRatio = 0.5f * (float)chartWidth / (float)view.state->plotHeight;
                    ImPlot::SetupAxesLimits(0.5 - aspectRatio, 0.5 + aspectRatio, 0, 1, ImPlotCond_Always);

                    auto const totalTime = (view.state->pieChart.cpuTotal ? node.cpuTime : node.gpuTime);
                    render_pie_chart_node(node, 0, view.state->pieChart.levels, 0.4, 90.0, totalTime);
                    ImPlot::EndPlot();
                }
                ImPlot::PopColormap();
            }
        }
    }

    auto ElementProfiler::render_pie_chart_node(EntryNode const& node, int level, int numLevels, double plotRadius, double angle0, double totalTime) -> void
    {
        std::vector<const char*> labels(node.child.size());
        std::vector<float> data(node.child.size());
        for(size_t i = 0; i < node.child.size(); i++) {
            labels[i] = node.child[i].name.c_str();
            data[i] = static_cast<float>(node.child[i].gpuTime / totalTime);
        }

        auto const myRadius = numLevels == 1 ? plotRadius : plotRadius * (1.0 - (0.5 * level) / (numLevels - 1));
        auto const* text = (level + 1 == numLevels ? "%.2f" : "");
        ImPlot::PlotPieChart(labels.data(), data.data(), static_cast<int>(data.size()), 0.5, 0.5, myRadius, text, angle0);

        if(level + 1 < numLevels && !node.child.empty()) {
            for(size_t i = 0; i < node.child.size(); i++) {
                render_pie_chart_node(node.child[i], level + 1, numLevels, plotRadius, angle0, totalTime);
                angle0 += 360.0 * node.child[i].gpuTime / totalTime;
            }
        }
    }

    auto ElementProfiler::render_bar_chart(View& view) -> void
    {
        // Grid Mode Logic
        const bool gridMode = ImGui::GetContentRegionAvail().x >= 600 && m_frameNodes.size() > 1;
        const float width   = ImGui::GetContentRegionAvail().x / (gridMode ? 2.0f : 1.0f) - 5.0f;

        draw_vsync_checkbox();
        ImGui::SameLine();
        ImGui::CheckboxFlags("Stacked", (unsigned int*)&view.state->barChart.stacked, ImPlotBarGroupsFlags_Stacked);

        for(size_t i = 0; i < m_frameNodes.size(); ++i)
        {
            auto const& rootNode = m_frameNodes[i];
            if(!rootNode.child.empty())
            {
                auto const& node = rootNode.child[0];
                int items = 0;

                std::vector<const char*> labels(node.child.size());
                std::vector<float> data(node.child.size());
                for(size_t k = 0; k < node.child.size(); k++) {
                    items++;
                    labels[k] = node.child[k].name.c_str();
                    data[k] = node.child[k].gpuTime;
                }

                if(gridMode && i % 2 != 0) ImGui::SameLine();

                if(i % 3 == 0) ImPlot::PushColormap(ImPlotColormap_Deep);
                else if(i % 3 == 1) ImPlot::PushColormap(ImPlotColormap_Pastel);
                else ImPlot::PushColormap(ImPlotColormap_Viridis);

                if(ImPlot::BeginPlot(rootNode.name.c_str(), ImVec2(width, (float)view.state->plotHeight), ImPlotFlags_NoMouseText)) {
                    ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
                    ImPlot::SetupAxes("Time (ms)", "Timers", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoGridLines);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, -0.4, 0.6, ImPlotCond_Always);

                    if(items > 0)
                        ImPlot::PlotBarGroups(labels.data(), data.data(), items, 1, 0.67, 0, (ImPlotBarGroupsFlags)view.state->barChart.stacked | ImPlotBarGroupsFlags_Horizontal);

                    ImPlot::EndPlot();
                }
                ImPlot::PopColormap();
            }
        }
    }

    auto ElementProfiler::render_line_chart(View& view) -> void
    {
        // Grid Mode Calculation
        const bool gridMode = ImGui::GetContentRegionAvail().x >= 600 && m_frameNodes.size() > 1;
        const float width   = ImGui::GetContentRegionAvail().x / (gridMode ? 2.0f : 1.0f) - 5.0f;

        draw_vsync_checkbox();
        ImGui::SameLine();
        ImGui::Checkbox("CPU line", &view.state->lineChart.cpuLine);
        ImGui::SameLine(); ImGui::Checkbox("GPU lines", &view.state->lineChart.gpuLines);
        ImGui::SameLine(); ImGui::Checkbox("GPU fills", &view.state->lineChart.gpuFills);

        for(size_t i = 0; i < m_frameNodes.size(); ++i)
        {
            auto const& rootNode = m_frameNodes[i];
            if(!rootNode.child.empty())
            {
                auto const& node = rootNode.child[0];

                // Data Gathering & Stacking Logic
                std::vector<const char*> gpuTimesLabels(node.child.size());
                std::vector<std::vector<float>> gpuTimes(node.child.size());
                std::vector<float> cpuTimes(node.timerInfo.numAveraged);
                float avgCpuTime = 0.f;
                float avgGpuTime = 0.f;

                // Fill GPU times
                for(size_t k = 0; k < node.child.size(); k++)
                {
                    const auto& child = node.child[k];
                    float gpuTimeSum = 0.f;
                    gpuTimesLabels[k] = child.name.c_str();

                    if(!child.timerInfo.gpu.times.empty())
                    {
                        gpuTimes[k].resize(child.timerInfo.numAveraged);
                        for(size_t j = 0; j < child.timerInfo.numAveraged; j++)
                        {
                            // Ring buffer access
                            uint32_t index = (child.timerInfo.gpu.index - child.timerInfo.numAveraged + j) % TimerStats::kMaxLastFrames;
                            gpuTimes[k][j] = float(child.timerInfo.gpu.times[index] / 1000.0);

                            // Stacking!
                            if(k > 0) gpuTimes[k][j] += gpuTimes[k-1][j];

                            gpuTimeSum += gpuTimes[k][j]; 
                        }
                        if(child.timerInfo.numAveraged != 0)
                            avgGpuTime += gpuTimeSum / child.timerInfo.numAveraged;
                    }
                }

                // Fill CPU times
                for(size_t j = 0; j < node.timerInfo.numAveraged; j++)
                {
                    uint32_t index = (node.timerInfo.cpu.index - node.timerInfo.numAveraged + j) % TimerStats::kMaxLastFrames;
                    cpuTimes[j] = float(node.timerInfo.cpu.times[index] / 1000.0);
                    avgCpuTime += cpuTimes[j];
                }

                // Temporal Smoothing for Y-Axis
                float avgTime = 0.f;
                if(node.timerInfo.numAveraged > 0)
                {
                    avgCpuTime /= node.timerInfo.numAveraged;
                    avgTime = view.state->lineChart.cpuLine ? avgCpuTime : avgGpuTime;
                }

                if(view.maxY == 0.f) {
                    view.maxY = avgTime;
                } else {
                    const float TEMPORAL_SMOOTHING = 20.f;
                    view.maxY = (TEMPORAL_SMOOTHING * view.maxY + avgTime) / (TEMPORAL_SMOOTHING + 1.f);
                }

                // Plotting
                if(!gpuTimes.empty() && !gpuTimes[0].empty())
                {
                    if(gridMode && i % 2 != 0) ImGui::SameLine();

                    if(i % 3 == 0) ImPlot::PushColormap(ImPlotColormap_Deep);
                    else if(i % 3 == 1) ImPlot::PushColormap(ImPlotColormap_Pastel);
                    else ImPlot::PushColormap(ImPlotColormap_Viridis);

                    const ImPlotFlags plotFlags = ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText | ImPlotFlags_Crosshairs;
                    const ImPlotAxisFlags axesFlags = ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoLabel;

                    if(ImPlot::BeginPlot(rootNode.name.c_str(), ImVec2(width, (float)view.state->plotHeight), plotFlags))
                    {
                        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
                        ImPlot::SetupAxes(nullptr, "Count", axesFlags | ImPlotAxisFlags_NoTickLabels, axesFlags);
                        ImPlot::SetupAxesLimits(0, node.child[0].timerInfo.numAveraged, 0, view.maxY * 1.2, ImPlotCond_Always);

                        if(view.state->lineChart.cpuLine) {
                            ImPlot::SetNextLineStyle(ImColor(1.f, 0.f, 0.f, 1.0f), 0.1f);
                            ImPlot::PlotLine("CPU", cpuTimes.data(), (int)cpuTimes.size());
                        }

                        // Draw GPU Stack (Backwards to Front)
                        for(size_t k = 0; k < node.child.size(); k++)
                        {
                            size_t index = node.child.size() - k - 1;
                            if(view.state->lineChart.gpuFills) {
                                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                                ImPlot::PlotShaded(node.child[index].name.c_str(), gpuTimes[index].data(), (int)gpuTimes[index].size(), -INFINITY);
                                ImPlot::PopStyleVar();
                            }
                            if(view.state->lineChart.gpuLines) {
                                ImPlot::PlotLine(node.child[index].name.c_str(), gpuTimes[index].data(), (int)gpuTimes[index].size());
                            }
                        }

                        // Tooltip Logic
                        if(ImPlot::IsPlotHovered())
                        {
                            ImPlotPoint mouse = ImPlot::GetPlotMousePos();
                            int mouseOffset = (int(mouse.x)) % (int)gpuTimes[0].size();
                            if (mouseOffset < 0) mouseOffset = 0;
                            if (mouseOffset >= (int)gpuTimes[0].size()) mouseOffset = (int)gpuTimes[0].size() - 1;

                            std::vector<float> localTimes(node.child.size());
                            ImGui::BeginTooltip();

                            if (mouseOffset < (int)cpuTimes.size())
                                ImGui::Text("CPU: %.3f ms", cpuTimes[mouseOffset]);

                            float totalGpu = 0.f;
                            for(size_t k = 0; k < node.child.size(); k++)
                            {
                                if(k == 0) localTimes[k] = gpuTimes[k][mouseOffset];
                                else localTimes[k] = gpuTimes[k][mouseOffset] - gpuTimes[k-1][mouseOffset];
                                totalGpu += localTimes[k];
                            }
                            ImGui::Text("GPU: %.3f ms", totalGpu);
                            for(size_t k = 0; k < node.child.size(); k++) {
                                ImGui::Text("  %s: %.3f ms (%.1f%%)", node.child[k].name.c_str(), localTimes[k], localTimes[k] * 100.f / (totalGpu > 0 ? totalGpu : 1.f));
                            }
                            ImGui::EndTooltip();
                        }

                        ImPlot::EndPlot();
                    }
                    ImPlot::PopColormap();
                }
            }
        }
    }

    auto ElementProfiler::isVsync() const -> bool
    {
        return m_vsync;
    }

    auto ElementProfiler::setVsync(bool vsync) -> void
    {
        m_vsync = vsync;
    }

    auto ElementProfiler::draw_vsync_checkbox() -> void
    {
        auto vsync = isVsync();
        bool showRed = vsync;

        if(showRed) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
        if (ImGui::Checkbox("V-Sync", &vsync)) {
            setVsync(vsync);
        }
        if(showRed) ImGui::PopStyleColor();

        if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Disable V-Sync to measure nominal performance.");
    }
}