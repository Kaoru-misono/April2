// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace april::graphics
{
    /**
     * A graph structure used to map state transitions to unique objects.
     *
     * Used primarily for deduping pipeline states or program versions.
     */
    template <typename NodeType, typename EdgeType, typename EdgeHashType = std::hash<EdgeType>>
    class StateGraph
    {
    public:
        using CompareFunc = std::function<bool(NodeType const& data)>;

        StateGraph() : m_graph(1) {}

        auto hasEdge(EdgeType const& e) const -> bool
        {
            return (getEdgeIter(e) != m_graph[m_currentNode].edges.end());
        }

        auto walk(EdgeType const& e) -> bool
        {
            if (hasEdge(e))
            {
                m_currentNode = getEdgeIter(e)->second;
                return true;
            }
            else
            {
                // Create a new node
                auto newIndex = static_cast<uint32_t>(m_graph.size());
                m_graph[m_currentNode].edges[e] = newIndex;
                m_graph.emplace_back(Node());
                m_currentNode = newIndex;
                return false;
            }
        }

        auto getCurrentNode() const -> NodeType const&
        {
            return m_graph[m_currentNode].data;
        }

        auto setCurrentNodeData(NodeType const& data) -> void
        {
            m_graph[m_currentNode].data = data;
        }

        /**
         * Scans existing nodes to see if one matches the current node's data (using cmpFunc).
         * If a match is found, it "collapses" the graph by redirecting all edges pointing
         * to the current (temporary) node to the matching (existing) node.
         */
        auto scanForMatchingNode(CompareFunc cmpFunc) -> bool
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(m_graph.size()); i++)
            {
                if (i != m_currentNode)
                {
                    if (cmpFunc(m_graph[i].data))
                    {
                        // Found a match at index 'i'.
                        // Reconnect any edge that points to m_currentNode to point to 'i' instead.
                        for (uint32_t n = 0; n < static_cast<uint32_t>(m_graph.size()); n++)
                        {
                            for (auto& e : m_graph[n].edges)
                            {
                                if (e.second == m_currentNode)
                                {
                                    e.second = i;
                                }
                            }
                        }

                        // Move our current pointer to the existing node
                        m_currentNode = i;
                        return true;
                    }
                }
            }
            return false;
        }

    private:
        using EdgeMap = std::unordered_map<EdgeType, uint32_t, EdgeHashType>;

        struct Node
        {
            NodeType data{}; // Default initialization
            EdgeMap edges{};
        };

        auto getEdgeIter(EdgeType const& e) const -> typename EdgeMap::const_iterator
        {
            Node const& n = m_graph[m_currentNode];
            return n.edges.find(e);
        }

        std::vector<Node> m_graph;
        uint32_t m_currentNode{0};
    };
} // namespace april::graphics
