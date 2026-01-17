// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "vertex-layout.hpp"
#include "buffer.hpp"

#include <core/foundation/object.hpp>
#include <vector>

namespace april::graphics
{
    class VertexArrayObject : public core::Object
    {
        APRIL_OBJECT(VertexArrayObject)
    public:
        enum class Topology
        {
            Undefined,
            PointList,
            LineList,
            LineStrip,
            TriangleList,
            TriangleStrip
        };

        struct ElementDesc
        {
            static constexpr uint32_t kInvalidIndex = -1;
            uint32_t vbIndex{kInvalidIndex};
            uint32_t elementIndex{kInvalidIndex};
        };

        using BufferVec = std::vector<core::ref<Buffer>>;

        virtual ~VertexArrayObject() = default;

        static auto create(
            Topology primTopology,
            core::ref<VertexLayout> layout = nullptr,
            BufferVec const& vbs = BufferVec(),
            core::ref<Buffer> ib = nullptr,
            ResourceFormat ibFormat = ResourceFormat::Unknown
        ) -> core::ref<VertexArrayObject>;

        auto getVertexBuffersCount() const -> uint32_t { return (uint32_t)mp_vertexBuffers.size(); }

        auto getVertexBuffer(uint32_t index) const -> core::ref<Buffer> const&
        {
            return mp_vertexBuffers[index];
        }

        auto getVertexLayout() const -> core::ref<VertexLayout> const& { return mp_vertexLayout; }

        auto getElementIndexByLocation(uint32_t elementLocation) const -> ElementDesc;

        auto getIndexBuffer() const -> core::ref<Buffer> const& { return mp_indexBuffer; }
        auto getIndexBufferFormat() const -> ResourceFormat { return m_indexFormat; }
        auto getPrimitiveTopology() const -> Topology { return m_topology; }

    protected:
        friend class CommandContext;

    private:
        VertexArrayObject(BufferVec const& vbs, core::ref<VertexLayout> layout, core::ref<Buffer> ib, ResourceFormat ibFormat, Topology primTopology);

        core::ref<VertexLayout> mp_vertexLayout{};
        BufferVec mp_vertexBuffers{};
        core::ref<Buffer> mp_indexBuffer{};
        // TODO: offset.
        void* m_privateData{nullptr};
        ResourceFormat m_indexFormat{};
        Topology m_topology{};
    };
}
