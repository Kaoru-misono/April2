// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "ray-tracing-acceleration-structure-post-build-info-pool.hpp"
#include "buffer.hpp"
#include "resource.hpp"

#include <core/math/type.hpp>
#include <core/tools/enum-flags.hpp>
#include <core/foundation/object.hpp>
#include <slang-com-ptr.h>
#include <vector>

namespace april::graphics
{
    class Device;

    // Constants
    const uint64_t kAccelerationStructureByteAlignment = 256;

    using DeviceAddress = uint64_t;

    enum class RtGeometryInstanceFlags : uint8_t
    {
        None = 0,
        TriangleFacingCullDisable = 0x00000001,
        TriangleFrontCounterClockwise = 0x00000002,
        ForceOpaque = 0x00000004,
        NoOpaque = 0x00000008,
    };
    AP_ENUM_CLASS_OPERATORS(RtGeometryInstanceFlags);

    struct RtInstanceDesc
    {
        float transform[3][4]{};
        uint32_t instanceID : 24{};
        uint32_t instanceMask : 8{};
        uint32_t instanceContributionToHitGroupIndex : 24{};
        RtGeometryInstanceFlags flags : 8{};
        DeviceAddress accelerationStructure{};

        auto setTransform(float4x4 const& matrix) -> RtInstanceDesc&;
    };

    enum class RtAccelerationStructureKind
    {
        TopLevel,
        BottomLevel
    };

    enum class RtAccelerationStructureBuildFlags : uint32_t
    {
        None = 0,
        AllowUpdate = 1,
        AllowCompaction = 2,
        PreferFastTrace = 4,
        PreferFastBuild = 8,
        MinimizeMemory = 16,
        PerformUpdate = 32
    };
    AP_ENUM_CLASS_OPERATORS(RtAccelerationStructureBuildFlags);

    enum class RtGeometryType
    {
        Triangles,
        ProcedurePrimitives
    };

    enum class RtGeometryFlags : uint32_t
    {
        None = 0,
        Opaque = 1,
        NoDuplicateAnyHitInvocation = 2
    };
    AP_ENUM_CLASS_OPERATORS(RtGeometryFlags);

    struct RtTriangleDesc
    {
        DeviceAddress transform3x4{};
        ResourceFormat indexFormat{};
        ResourceFormat vertexFormat{};
        uint32_t indexCount{};
        uint32_t vertexCount{};
        // TODO: Use vertex buffer and index buffer?
        DeviceAddress indexData{};
        DeviceAddress vertexData{};
        uint64_t vertexStride{};
    };

    struct RtAABBDesc
    {
        uint64_t count{};
        DeviceAddress data{};
        uint64_t stride{};
    };

    struct RtGeometryDesc
    {
        RtGeometryType type{};
        RtGeometryFlags flags{};
        union
        {
            RtTriangleDesc triangles;
            RtAABBDesc proceduralAABBs;
        } content{};
    };

    struct RtAccelerationStructurePrebuildInfo
    {
        uint64_t resultDataMaxSize{};
        uint64_t scratchDataSize{};
        uint64_t updateScratchDataSize{};
    };

    struct RtAccelerationStructureBuildInputs
    {
        RtAccelerationStructureKind kind{};
        RtAccelerationStructureBuildFlags flags{};
        uint32_t descCount{};
        DeviceAddress instanceDescs{};
        RtGeometryDesc const* geometryDescs{};
    };

    class RtAccelerationStructure : public core::Object
    {
        APRIL_OBJECT(RtAccelerationStructure)
    public:
        class Desc
        {
        public:
            friend class RtAccelerationStructure;

            auto setKind(RtAccelerationStructureKind kind) -> Desc&
            {
                m_kind = kind;
                return *this;
            }

            auto setBuffer(core::ref<Buffer> buffer, uint64_t offset, uint64_t size) -> Desc&
            {
                m_buffer = buffer;
                m_offset = offset;
                m_size = size;
                return *this;
            }

            auto getBuffer() const -> core::ref<Buffer> { return m_buffer; }
            auto getOffset() const -> uint64_t { return m_offset; }
            auto getSize() const -> uint64_t { return m_size; }
            auto getKind() const -> RtAccelerationStructureKind { return m_kind; }

        protected:
            RtAccelerationStructureKind m_kind{RtAccelerationStructureKind::BottomLevel};
            core::ref<Buffer> m_buffer{nullptr};
            uint64_t m_offset{0};
            uint64_t m_size{0};
        };

        struct BuildDesc
        {
            RtAccelerationStructureBuildInputs inputs{};
            RtAccelerationStructure* source{};
            RtAccelerationStructure* dest{};
            DeviceAddress scratchData{};
        };

        static auto create(core::ref<Device> device, Desc const& desc) -> core::ref<RtAccelerationStructure>;
        static auto getPrebuildInfo(Device* device, RtAccelerationStructureBuildInputs const& inputs) -> RtAccelerationStructurePrebuildInfo;

        virtual ~RtAccelerationStructure();

        auto getGpuAddress() -> uint64_t;
        Desc const& getDesc() const { return m_desc; }
        auto getGfxAccelerationStructure() const -> rhi::IAccelerationStructure* { return m_gfxAccelerationStructure.get(); }

    protected:
        RtAccelerationStructure(core::ref<Device> device, Desc const& desc);

        core::ref<Device> mp_device{};
        Desc m_desc{};
        Slang::ComPtr<rhi::IAccelerationStructure> m_gfxAccelerationStructure{};
    };

    struct GFXAccelerationStructureBuildInputsTranslator
    {
    public:
        auto translate(RtAccelerationStructureBuildInputs const& buildInputs) -> std::vector<rhi::AccelerationStructureBuildInput> const&;

    private:
        std::vector<rhi::AccelerationStructureBuildInput> m_inputs{};

        auto translateGeometryFlags(RtGeometryFlags flags) -> rhi::AccelerationStructureGeometryFlags
        {
            return (rhi::AccelerationStructureGeometryFlags)flags;
        }
    };

    auto getGFXAccelerationStructurePostBuildQueryType(RtAccelerationStructurePostBuildInfoQueryType type) -> rhi::QueryType;

} // namespace april::graphics
