// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "ray-tracing-acceleration-structure.hpp"
#include "render-device.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>

namespace april::graphics
{
    auto RtInstanceDesc::setTransform(float4x4 const& matrix) -> RtInstanceDesc&
    {
        std::memcpy(transform, &matrix, sizeof(transform));
        return *this;
    }

    auto RtAccelerationStructure::create(core::ref<Device> device, Desc const& desc) -> core::ref<RtAccelerationStructure>
    {
        return core::ref<RtAccelerationStructure>(new RtAccelerationStructure(device, desc));
    }

    RtAccelerationStructure::RtAccelerationStructure(core::ref<Device> device, Desc const& desc)
        : mp_device(device), m_desc(desc)
    {
        rhi::AccelerationStructureDesc createDesc = {};
        createDesc.size = m_desc.getSize();
        // TODO:
        createDesc.flags = {};
        createDesc.motionInfo = {};
        checkResult(mp_device->getGfxDevice()->createAccelerationStructure(createDesc, m_gfxAccelerationStructure.writeRef()), "Failed to create acceleration structure");
    }

    RtAccelerationStructure::~RtAccelerationStructure()
    {
        mp_device->releaseResource(m_gfxAccelerationStructure.get());
    }

    auto RtAccelerationStructure::getGpuAddress() -> uint64_t
    {
        return m_desc.m_buffer->getGpuAddress() + m_desc.m_offset;
    }

    auto RtAccelerationStructure::getPrebuildInfo(Device* device, RtAccelerationStructureBuildInputs const& inputs) -> RtAccelerationStructurePrebuildInfo
    {
        GFXAccelerationStructureBuildInputsTranslator translator;
        auto const& gfxBuildInputs = translator.translate(inputs);

        rhi::AccelerationStructureBuildDesc buildDesc = {};
        buildDesc.inputs = gfxBuildInputs.data();
        buildDesc.inputCount = static_cast<uint32_t>(gfxBuildInputs.size());
        buildDesc.flags = (rhi::AccelerationStructureBuildFlags)(inputs.flags & ~RtAccelerationStructureBuildFlags::PerformUpdate);

        rhi::AccelerationStructureSizes gfxSizes;
        checkResult(device->getGfxDevice()->getAccelerationStructureSizes(buildDesc, &gfxSizes), "Failed to get acceleration structure sizes");

        RtAccelerationStructurePrebuildInfo result = {};
        result.resultDataMaxSize = gfxSizes.accelerationStructureSize;
        result.scratchDataSize = gfxSizes.scratchSize;
        result.updateScratchDataSize = gfxSizes.updateScratchSize;
        return result;
    }

    auto GFXAccelerationStructureBuildInputsTranslator::translate(RtAccelerationStructureBuildInputs const& buildInputs) -> std::vector<rhi::AccelerationStructureBuildInput> const&
    {
        m_inputs.clear();

        if (buildInputs.kind == RtAccelerationStructureKind::TopLevel)
        {
            rhi::AccelerationStructureBuildInput input = {};
            input.type = rhi::AccelerationStructureBuildInputType::Instances;
            input.instances.instanceCount = buildInputs.descCount;
            input.instances.instanceBuffer.buffer = nullptr; // TODO: RtAccelerationStructureBuildInputs doesn't seem to have a buffer, only a DeviceAddress.
            input.instances.instanceBuffer.offset = buildInputs.instanceDescs;
            input.instances.instanceStride = sizeof(RtInstanceDesc);
            m_inputs.push_back(input);
        }
        else
        {
            // Bottom Level
            if (buildInputs.geometryDescs)
            {
                for (uint32_t i = 0; i < buildInputs.descCount; ++i)
                {
                    auto const& inputGeomDesc = buildInputs.geometryDescs[i];
                    rhi::AccelerationStructureBuildInput input = {};

                    switch (inputGeomDesc.type)
                    {
                    case RtGeometryType::Triangles:
                        input.type = rhi::AccelerationStructureBuildInputType::Triangles;
                        input.triangles.flags = translateGeometryFlags(inputGeomDesc.flags);
                        input.triangles.vertexBuffers[0].buffer = nullptr; // FIXME: Why nillptr?
                        input.triangles.vertexBuffers[0].offset = inputGeomDesc.content.triangles.vertexData;
                        input.triangles.vertexCount = inputGeomDesc.content.triangles.vertexCount;
                        input.triangles.vertexStride = static_cast<uint32_t>(inputGeomDesc.content.triangles.vertexStride);
                        input.triangles.vertexFormat = getGFXFormat(inputGeomDesc.content.triangles.vertexFormat);
                        input.triangles.vertexBufferCount = 1;

                        if (inputGeomDesc.content.triangles.indexCount > 0)
                        {
                            input.triangles.indexBuffer.buffer = nullptr;
                            input.triangles.indexBuffer.offset = inputGeomDesc.content.triangles.indexData;
                            input.triangles.indexCount = inputGeomDesc.content.triangles.indexCount;
                            // Need to convert ResourceFormat to rhi::IndexFormat
                            input.triangles.indexFormat = (inputGeomDesc.content.triangles.indexFormat == ResourceFormat::R32Uint) ? rhi::IndexFormat::Uint32 : rhi::IndexFormat::Uint16;
                        }

                        if (inputGeomDesc.content.triangles.transform3x4 != 0)
                        {
                            input.triangles.preTransformBuffer.buffer = nullptr;
                            input.triangles.preTransformBuffer.offset = inputGeomDesc.content.triangles.transform3x4;
                        }
                        break;

                    case RtGeometryType::ProcedurePrimitives:
                        input.type = rhi::AccelerationStructureBuildInputType::ProceduralPrimitives;
                        input.proceduralPrimitives.flags = translateGeometryFlags(inputGeomDesc.flags);
                        input.proceduralPrimitives.aabbBuffers[0].buffer = nullptr; // FIXME: Why nillptr?
                        input.proceduralPrimitives.aabbBuffers[0].offset = inputGeomDesc.content.proceduralAABBs.data;
                        input.proceduralPrimitives.aabbBufferCount = 1;
                        input.proceduralPrimitives.aabbStride = static_cast<uint32_t>(inputGeomDesc.content.proceduralAABBs.stride);
                        input.proceduralPrimitives.primitiveCount = static_cast<uint32_t>(inputGeomDesc.content.proceduralAABBs.count);
                        break;

                    default:
                        AP_UNREACHABLE();
                    }
                    m_inputs.push_back(input);
                }
            }
        }

        return m_inputs;
    }

    auto getGFXAccelerationStructurePostBuildQueryType(RtAccelerationStructurePostBuildInfoQueryType type) -> rhi::QueryType
    {
        switch (type)
        {
        case RtAccelerationStructurePostBuildInfoQueryType::CompactedSize:
            return rhi::QueryType::AccelerationStructureCompactedSize;
        case RtAccelerationStructurePostBuildInfoQueryType::SerializationSize:
            return rhi::QueryType::AccelerationStructureSerializedSize;
        case RtAccelerationStructurePostBuildInfoQueryType::CurrentSize:
            return rhi::QueryType::AccelerationStructureCurrentSize;
        default:
            AP_UNREACHABLE();
            return rhi::QueryType::AccelerationStructureCompactedSize;
        }
    }
} // namespace april::graphics
