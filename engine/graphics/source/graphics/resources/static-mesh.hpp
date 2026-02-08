// This file is part of the April Engine.

#pragma once

#include "rhi/vertex-array-object.hpp"

#include <asset/blob-header.hpp>
#include <core/foundation/object.hpp>
#include <vector>
#include <array>

namespace april::graphics
{
    /**
     * StaticMesh holds GPU buffers and submesh data for a static mesh asset.
     */
    class StaticMesh : public core::Object
    {
        APRIL_OBJECT(StaticMesh)
    public:
        /**
         * Submesh draw range for multi-material support.
         */
        struct DrawRange
        {
            uint32_t indexOffset = 0;
            uint32_t indexCount = 0;
            uint32_t materialIndex = 0;
        };

        StaticMesh(
            core::ref<VertexArrayObject> vao,
            std::vector<DrawRange> submeshes,
            std::array<float, 3> boundsMin,
            std::array<float, 3> boundsMax
        )
            : mp_vao{std::move(vao)}
            , m_submeshes{std::move(submeshes)}
            , m_boundsMin{boundsMin}
            , m_boundsMax{boundsMax}
        {}

        virtual ~StaticMesh() = default;

        [[nodiscard]] auto getVAO() const -> core::ref<VertexArrayObject> const& { return mp_vao; }
        [[nodiscard]] auto getSubmeshCount() const -> size_t { return m_submeshes.size(); }
        [[nodiscard]] auto getSubmesh(size_t index) const -> DrawRange const& { return m_submeshes[index]; }
        [[nodiscard]] auto getSubmeshes() const -> std::vector<DrawRange> const& { return m_submeshes; }
        [[nodiscard]] auto getBoundsMin() const -> std::array<float, 3> const& { return m_boundsMin; }
        [[nodiscard]] auto getBoundsMax() const -> std::array<float, 3> const& { return m_boundsMax; }

    private:
        core::ref<VertexArrayObject> mp_vao{};
        std::vector<DrawRange> m_submeshes{};
        std::array<float, 3> m_boundsMin{};
        std::array<float, 3> m_boundsMax{};
    };

} // namespace april::graphics
