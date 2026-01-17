// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "fwd.hpp"

#include <core/foundation/object.hpp>
#include <core/error/assert.hpp>
#include <slang-rhi.h>

#include <deque>
#include <string>

namespace april::graphics
{
    class QueryHeap : public core::Object
    {
        APRIL_OBJECT(QueryHeap)
    public:
        enum class Type
        {
            Timestamp,
            Occlusion,
            PipelineStats
        };

        static constexpr uint32_t kInvalidIndex = 0xffffffff;

        /**
         * Create a new query heap.
         * @param[in] p_device Device to create the heap on.
         * @param[in] type Type of queries.
         * @param[in] count Number of queries.
         * @return New object, or throws an exception if creation failed.
         */
        static auto create(core::ref<Device> p_device, Type type, uint32_t count) -> core::ref<QueryHeap>;

        auto getGfxQueryPool() const -> rhi::IQueryPool* { return m_gfxQueryPool.get(); }
        auto getQueryCount() const -> uint32_t { return m_count; }
        auto getType() const -> Type { return m_type; }

        /**
         * Allocates a new query.
         * @return Query index, or kInvalidIndex if out of queries.
         */
        auto allocate() -> uint32_t
        {
            if (m_freeQueries.size())
            {
                uint32_t entry = m_freeQueries.front();
                m_freeQueries.pop_front();
                return entry;
            }
            if (m_currentObject < m_count)
                return m_currentObject++;
            else
                return kInvalidIndex;
        }

        auto release(uint32_t entry) -> void
        {
            AP_ASSERT(entry != kInvalidIndex, "Releasing invalid query index");
            m_freeQueries.push_back(entry);
        }

        auto breakStrongReferenceToDevice() -> void;

    private:
        QueryHeap(core::ref<Device> p_device, Type type, uint32_t count);

        core::BreakableReference<Device> mp_device;
        Slang::ComPtr<rhi::IQueryPool> m_gfxQueryPool {};
        uint32_t m_count {};
        uint32_t m_currentObject {};
        std::deque<uint32_t> m_freeQueries {};
        Type m_type {};
    };

    inline auto to_string(QueryHeap::Type type) -> std::string
    {
    #define item_to_string(item)      \
        case QueryHeap::Type::item: \
            return #item;
        switch (type)
        {
            item_to_string(Timestamp);
            item_to_string(Occlusion);
            item_to_string(PipelineStats);
        default:
            return "Unknown";
        }
    #undef item_to_string
    }

} // namespace april::graphics
