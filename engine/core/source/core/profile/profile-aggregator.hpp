#pragma once

#include "profile-types.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace april::core
{
    struct ProfileNode
    {
        std::string name{};
        double lastUs{0.0};
        double avgUs{0.0};
        double minUs{0.0};
        double maxUs{0.0};
        std::vector<ProfileNode> children{};
    };

    struct ProfileThreadFrame
    {
        uint32_t threadId{0};
        std::string threadName{};
        std::vector<ProfileNode> roots{};
    };

    class ProfileAggregator
    {
    public:
        auto ingest(
            std::vector<ProfileEvent> const& events,
            std::map<uint32_t, std::string> const& threadNames
        ) -> void;

        auto getFrames() const -> std::vector<ProfileThreadFrame> const& { return m_frames; }

        auto clear() -> void;

    private:
        struct StatHistory
        {
            double totalUs{0.0};
            double minUs{0.0};
            double maxUs{0.0};
            uint64_t count{0};
        };

        struct StackEntry
        {
            double endUs{0.0};
            ProfileNode* node{nullptr};
        };

        auto buildFrames(
            std::vector<ProfileEvent> const& events,
            std::map<uint32_t, std::string> const& threadNames
        ) -> void;
        auto updateStats(ProfileThreadFrame& frame) -> void;
        auto updateNodeStats(ProfileNode& node, std::string const& path, uint32_t threadId) -> void;
        auto findOrCreateChild(std::vector<ProfileNode>& nodes, std::string_view name) -> ProfileNode*;

        std::vector<ProfileThreadFrame> m_frames{};
        std::unordered_map<std::string, StatHistory> m_history{};
    };
}
