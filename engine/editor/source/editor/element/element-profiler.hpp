#pragma once

#include <ui/element.hpp>
#include <core/profile/profile-aggregator.hpp>
#include <core/profile/profile-manager.hpp>
#include <imgui.h>

#include <unordered_map>
#include <unordered_set>

namespace april::ui
{
    class ElementProfiler : public IElement
    {
        APRIL_OBJECT(ElementProfiler)
    public:
        ElementProfiler(bool show = false);
        ~ElementProfiler() override = default;

        auto onAttach(ImGuiLayer* pLayer) -> void override;
        auto onDetach() -> void override;
        auto onResize(graphics::CommandContext* pContext, float2 const& size) -> void override;
        auto onUIRender() -> void override;
        auto onUIMenu() -> void override;
        auto onPreRender() -> void override;
        auto onRender(graphics::CommandContext* pContext) -> void override;
        auto onFileDrop(std::filesystem::path const& filename) -> void override;

    private:
        auto draw() -> void;
        auto drawThread(april::core::ProfileThreadFrame& frame) -> void;
        auto drawNode(april::core::ProfileNode& node, std::string const& path) -> bool;
        auto nodeMatchesFilter(april::core::ProfileNode const& node) const -> bool;
        auto getOpenState(std::string const& path) const -> bool;
        auto setOpenState(std::string const& path, bool open) -> void;

        ImGuiTextFilter m_filter;
        bool m_show{false};
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
