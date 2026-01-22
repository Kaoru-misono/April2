// #pragma once
//
// #include <ui/element.hpp>
// #include <core/profile/profiler.hpp>
// #include <imgui.h>
// #include <implot.h>
// #include <memory>
// #include <vector>
// #include <string>
//
// namespace april::ui
// {
//     class ElementProfiler : public IElement
//     {
//         APRIL_OBJECT(ElementProfiler)
//     public:
//         enum class TabId: uint8_t
//         {
//             Table,
//             BarChart,
//             PieChart,
//             LineChart
//         };
//
//         struct ViewSettings {
//             std::string name = "Profiler";
//             bool show = true;
//             TabId defaultTab = TabId::Table;
//             int plotHeight = 250;
//             bool useAverage = true;
//             struct {
//                 bool detailed = false;
//                 uint32_t levels = ~0u;
//             } table;
//             struct {
//                 int stacked = 0; // Using int is fine for ImGui::CheckboxFlags
//             } barChart;
//             struct {
//                 bool cpuTotal = true;
//                 int levels = 1;
//             } pieChart;
//             struct {
//                 bool cpuLine = true;
//                 bool gpuLines = true;
//                 bool gpuFills = true;
//             } lineChart;
//         };
//
//         ElementProfiler(std::shared_ptr<ViewSettings> defaultSettings = nullptr);
//         virtual ~ElementProfiler() = default;
//
//         // IElement overrides
//         auto onAttach(ImGuiLayer* pLayer) -> void override;
//         auto onDetach() -> void override;
//         auto onResize(graphics::CommandContext* pContext, float2 const& size) -> void override;
//         auto onUIRender() -> void override;
//         auto onUIMenu() -> void override;
//         auto onPreRender() -> void override;
//         auto onRender(graphics::CommandContext* pContext) -> void override;
//         auto onFileDrop(std::filesystem::path const& filename) -> void override;
//
//         auto addView(std::shared_ptr<ViewSettings> state) -> void;
//
//         auto isVsync() const -> bool;
//         auto setVsync(bool vsync) -> void;
//
//     private:
//         struct View {
//             float maxY = 0.0f;
//             bool selectDefaultTab = true;
//             std::shared_ptr<ViewSettings> state;
//         };
//
//         struct EntryNode {
//             std::string name;
//             std::string apiName;
//             float cpuTime = 0.f;
//             float gpuTime = -1.f;
//             std::vector<EntryNode> child;
//             april::core::TimerInfo timerInfo;
//             size_t timerIndex = 0;
//         };
//
//         auto updateData() -> void;
//         auto addEntries(april::core::Snapshot const& snapshot,
//                             std::vector<EntryNode>& nodes,
//                             uint32_t startIndex, uint32_t endIndex, uint32_t currentLevel) -> uint32_t;
//
//         auto displayTableNode(EntryNode const& node, bool detailed, bool useAverage, bool showApi, uint32_t defaultOpenLevels, uint32_t depth) -> void;
//
//         auto renderTable(View& view) -> void;
//         auto renderPieChart(View& view) -> void;
//         auto renderBarChart(View& view) -> void;
//         auto renderLineChart(View& view) -> void;
//
//         auto renderPieChartNode(EntryNode const& node, int level, int numLevels, double plotRadius, double angle0, double totalTime) -> void;
//
//         auto drawVsyncCheckbox() -> void;
//
//         auto addSettingsHandler() -> void;
//
//         april::core::Profiler* m_profiler = nullptr;
//         std::vector<View> m_views;
//
//         char m_filter[128] = {0};
//
//         // Data Cache
//         std::vector<EntryNode> m_frameNodes;
//         std::vector<EntryNode> m_singleNodes;
//         std::vector<april::core::Snapshot> m_frameSnapshots;
//         std::vector<april::core::Snapshot> m_singleSnapshots;
//
//         bool m_show = false;
//         bool m_vsync = true;
//     };
// }
