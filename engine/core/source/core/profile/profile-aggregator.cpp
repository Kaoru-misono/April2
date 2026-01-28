#include "profile-aggregator.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <map>

namespace april::core
{
    auto ProfileAggregator::clear() -> void
    {
        m_frames.clear();
        m_history.clear();
    }

    auto ProfileAggregator::ingest(
        std::vector<ProfileEvent> const& events,
        std::map<uint32_t, std::string> const& threadNames
    ) -> void
    {
        buildFrames(events, threadNames);
        std::sort(m_frames.begin(), m_frames.end(), [](ProfileThreadFrame const& a, ProfileThreadFrame const& b) {
            if (a.threadName != b.threadName)
            {
                return a.threadName < b.threadName;
            }
            return a.threadId < b.threadId;
        });

        auto sortNodes = [](auto&& self, std::vector<ProfileNode>& nodes) -> void {
            std::sort(nodes.begin(), nodes.end(), [](ProfileNode const& a, ProfileNode const& b) {
                return a.name < b.name;
            });
            for (auto& node : nodes)
            {
                if (!node.children.empty())
                {
                    self(self, node.children);
                }
            }
        };
        for (auto& frame : m_frames)
        {
            if (!frame.roots.empty())
            {
                sortNodes(sortNodes, frame.roots);
            }
            updateStats(frame);
        }
    }

    auto ProfileAggregator::buildFrames(
        std::vector<ProfileEvent> const& events,
        std::map<uint32_t, std::string> const& threadNames
    ) -> void
    {
        m_frames.clear();

        std::map<uint32_t, std::vector<ProfileEvent const*>> perThread;
        for (auto const& event : events)
        {
            if (event.type != ProfileEventType::Complete)
            {
                continue;
            }
            perThread[event.threadId].push_back(&event);
        }

        for (auto const& [tid, list] : perThread)
        {
            ProfileThreadFrame frame{};
            frame.threadId = tid;

            auto nameIt = threadNames.find(tid);
            if (nameIt != threadNames.end())
            {
                frame.threadName = nameIt->second;
            }
            else
            {
                frame.threadName = "Thread " + std::to_string(tid);
            }

            std::vector<StackEntry> stack;
            stack.reserve(64);

            for (auto const* event : list)
            {
                const double startUs = event->timestamp;
                const double durationUs = std::max(0.0, event->duration);
                const double endUs = startUs + durationUs;

                while (!stack.empty() && startUs >= stack.back().endUs)
                {
                    stack.pop_back();
                }

                auto& siblings = stack.empty() ? frame.roots : stack.back().node->children;
                ProfileNode* node = findOrCreateChild(siblings, event->name ? event->name : "Unknown");
                node->lastUs += durationUs;

                if (durationUs > 0.0)
                {
                    stack.push_back({endUs, node});
                }
            }

            m_frames.emplace_back(std::move(frame));
        }
    }

    auto ProfileAggregator::updateStats(ProfileThreadFrame& frame) -> void
    {
        for (auto& node : frame.roots)
        {
            updateNodeStats(node, node.name, frame.threadId);
        }
    }

    auto ProfileAggregator::updateNodeStats(ProfileNode& node, std::string const& path, uint32_t threadId) -> void
    {
        auto key = std::to_string(threadId);
        key.append(":");
        key.append(path);

        auto& history = m_history[key];
        if (history.count == 0)
        {
            history.minUs = std::numeric_limits<double>::max();
            history.maxUs = 0.0;
        }

        if (node.lastUs > 0.0)
        {
            history.totalUs += node.lastUs;
            history.minUs = std::min(history.minUs, node.lastUs);
            history.maxUs = std::max(history.maxUs, node.lastUs);
            history.count += 1;
        }

        if (history.count > 0)
        {
            node.avgUs = history.totalUs / static_cast<double>(history.count);
            node.minUs = history.minUs;
            node.maxUs = history.maxUs;
        }

        for (auto& child : node.children)
        {
            auto childPath = path + "/" + child.name;
            updateNodeStats(child, childPath, threadId);
        }
    }

    auto ProfileAggregator::findOrCreateChild(std::vector<ProfileNode>& nodes, std::string_view name) -> ProfileNode*
    {
        for (auto& node : nodes)
        {
            if (node.name == name)
            {
                return &node;
            }
        }

        nodes.push_back(ProfileNode{.name = std::string(name)});
        return &nodes.back();
    }
}
