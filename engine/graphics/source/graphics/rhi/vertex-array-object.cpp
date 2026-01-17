// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "vertex-array-object.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>

namespace april::graphics
{
    VertexArrayObject::VertexArrayObject(BufferVec const& vbs, core::ref<VertexLayout> layout, core::ref<Buffer> ib, ResourceFormat ibFormat, Topology primTopology)
        : mp_vertexLayout(layout), mp_vertexBuffers(vbs), mp_indexBuffer(ib), m_indexFormat(ibFormat), m_topology(primTopology)
    {}

    auto VertexArrayObject::create(Topology primTopology, core::ref<VertexLayout> layout, BufferVec const& vbs, core::ref<Buffer> ib, ResourceFormat ibFormat) -> core::ref<VertexArrayObject>
    {
        // TODO: Check number of vertex buffers match with pLayout.
        AP_ASSERT(!ib || (ibFormat == ResourceFormat::R16Uint || ibFormat == ResourceFormat::R32Uint), "'ibFormat' must be R16Uint or R32Uint.");
        return core::ref<VertexArrayObject>(new VertexArrayObject(vbs, layout, ib, ibFormat, primTopology));
    }

    auto VertexArrayObject::getElementIndexByLocation(uint32_t elementLocation) const -> ElementDesc
    {
        ElementDesc desc;

        for (uint32_t bufId = 0; bufId < getVertexBuffersCount(); ++bufId)
        {
            auto const& pVbLayout = mp_vertexLayout->getBufferLayout(bufId);
            AP_ASSERT(pVbLayout);

            for (uint32_t i = 0; i < pVbLayout->getElementCount(); ++i)
            {
                if (pVbLayout->getElementShaderLocation(i) == elementLocation)
                {
                    desc.vbIndex = bufId;
                    desc.elementIndex = i;
                    return desc;
                }
            }
        }
        return desc;
    }
} // namespace april::graphics
