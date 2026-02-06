#pragma once

#include <editor/tool-window.hpp>
#include <core/profile/profile-aggregator.hpp>
#include <core/profile/profile-manager.hpp>
#include <imgui.h>

#include <unordered_map>
#include <unordered_set>

namespace april::editor
{
    class ProfilerWindow final : public ToolWindow
    {
    public:
        explicit ProfilerWindow(bool show = false);

        [[nodiscard]] auto title() const -> char const* override { return "Profiler"; }
        auto onUIRender(EditorContext& context) -> void override;

    private:
        auto draw() -> void;
        auto drawThread(april::core::ProfileThreadFrame& frame) -> void;
        auto drawNode(april::core::ProfileNode& node, std::string const& path) -> bool;
        auto nodeMatchesFilter(april::core::ProfileNode const& node) const -> bool;
        auto getOpenState(std::string const& path) const -> bool;
        auto setOpenState(std::string const& path, bool open) -> void;

        ImGuiTextFilter m_filter;
        bool m_paused{false};
        bool m_showAvg{true};
        bool m_showMinMax{true};

        april::core::ProfileAggregator m_aggregator{};
        std::vector<april::core::ProfileThreadFrame> m_frames{};
        std::unordered_map<std::string, bool> m_openState{};
        std::unordered_set<std::string> m_seenThisFrame{};
        std::unordered_set<std::string> m_seenLastFrame{};
    };
}
