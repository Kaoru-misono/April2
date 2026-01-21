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
        // m_profiler = GlobalProfiler::getManager(); // DISABLED
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
        // STUBBED: Waiting for april::core::ProfilerTimeline implementation in core module
    }

    auto ElementProfiler::display_table_node(EntryNode const& node, bool detailed, uint32_t defaultOpenLevels, uint32_t depth) -> void
    {
        // Stubbed
        ImGui::Text("Node: %s", node.name.c_str());
    }

    auto ElementProfiler::render_table(View& view) -> void
    {
         draw_vsync_checkbox();
         ImGui::SameLine();
         ImGui::Text("Profiler Table (Stubbed - Missing Core Dependencies)");
    }

    auto ElementProfiler::render_pie_chart(View& view) -> void {}
    auto ElementProfiler::render_bar_chart(View& view) -> void {}
    auto ElementProfiler::render_line_chart(View& view) -> void {}
    auto ElementProfiler::render_pie_chart_node(EntryNode const& node, int level, int numLevels, double plotRadius, double angle0, double totalTime) -> void {}

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