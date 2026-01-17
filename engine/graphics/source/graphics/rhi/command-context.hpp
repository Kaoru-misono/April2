#pragma once

#include "fwd.hpp"
#include "blit-context.hpp"
#include "graphics-pipeline.hpp"
#include "compute-pipeline.hpp"
#include "ray-tracing-pipeline.hpp"
#include "program/program.hpp"
#include "texture.hpp"
#include "sampler.hpp"
#include "ray-tracing-acceleration-structure.hpp"
#include "render-target.hpp"
#include "program/program-variables.hpp"
#include "tools/enum-flags.hpp"

#include <bitset>
#include <core/math/type.hpp>
#include <slang-rhi.h>
#include <string_view>

namespace april::graphics
{
    class CommandContext; // Forward declaration

    template<class TEncoder>
    class PassEncoderBase {
    protected:
        PassEncoderBase() = default;
        explicit PassEncoderBase(TEncoder* pEncoder)
            : m_encoder(pEncoder) {}

        auto pushDebugGroup(std::string_view name, float4 color = {1.0, 1.0f, 1.0f, 1.0f}) -> void;

        auto popDebugGroup() -> void;

        auto insertDebugMarker(std::string_view name, float4 color = {1.0, 1.0f, 1.0f, 1.0f}) -> void;

        auto writeTimestamp(QueryHeap* pHeap, uint32_t index) -> void;

        auto end() -> void;

        auto setEncoder(TEncoder* pEncoder) -> void { m_encoder = pEncoder; }

    protected:
        TEncoder* m_encoder{};
    };

    enum class RtAccelerationStructureCopyMode
    {
        Clone,
        Compact,
    };

    struct Viewport
    {
        float x{0.0f};
        float y{0.0f};
        float width{0.0f};
        float height{0.0f};
        float minDepth{0.0f};
        float maxDepth{1.0f};

        static auto fromSize(float pWidth, float pHeight, float pMinDepth = 0.0f, float pMaxDepth = 1.0f) -> Viewport
        {
            auto viewport = Viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = pWidth;
            viewport.height = pHeight;
            viewport.minDepth = pMinDepth;
            viewport.maxDepth = pMaxDepth;

            return viewport;
        }
    };

    struct Scissor
    {
        uint32_t offsetX{0};
        uint32_t offsetY{0};
        uint32_t extentX{0};
        uint32_t extentY{0};
    };

    class RenderPassEncoder final : public PassEncoderBase<rhi::IRenderPassEncoder>, public core::Object
    {
        APRIL_OBJECT(RenderPassEncoder)
    public:
        using PassEncoderBase::end;
        friend class CommandContext;

        RenderPassEncoder(
            CommandContext* pCtx,
            ColorTargets const& pColorTargets,
            DepthStencilTarget const& pDepthStencilTarget
        );

        static constexpr uint4 kMaxRect = {0, 0, std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max()};

        auto setViewport(uint32_t index, Viewport const& vp) -> void;
        auto setScissor(uint32_t index, Scissor const& sc) -> void;
        auto setVao(core::ref<VertexArrayObject> const& vao) -> void;

        // If program variables is nullptr, will bind the default ones from pipeline reflection.
        auto bindPipeline(GraphicsPipeline* pipeline, ProgramVariables* vars) -> void;

        auto draw(uint32_t vertexCount, uint32_t startVertexLocation) -> void;
        auto drawInstanced(
            uint32_t vertexCount,
            uint32_t instanceCount,
            uint32_t startVertexLocation,
            uint32_t startInstanceLocation
        ) -> void;

        auto drawIndexed(
            uint32_t indexCount,
            uint32_t startIndexLocation,
            int32_t baseVertexLocation
        ) -> void;

        auto drawIndexedInstanced(
            uint32_t indexCount,
            uint32_t instanceCount,
            uint32_t startIndexLocation,
            int32_t baseVertexLocation,
            uint32_t startInstanceLocation
        ) -> void;

        auto drawIndirect(
            uint32_t maxCommandCount,
            Buffer const* argBuffer,
            uint64_t argBufferOffset,
            Buffer const* countBuffer,
            uint64_t countBufferOffset
        ) -> void;

        auto drawIndexedIndirect(
            uint32_t maxCommandCount,
            Buffer const* argBuffer,
            uint64_t argBufferOffset,
            Buffer const* countBuffer,
            uint64_t countBufferOffset
        ) -> void;

        // Blit functions
        auto blit(
            core::ref<ShaderResourceView> const& src,
            core::ref<RenderTargetView> const& dst,
            uint4 srcRect = kMaxRect,
            uint4 dstRect = kMaxRect,
            TextureFilteringMode filter = TextureFilteringMode::Linear
        ) -> void;

        auto resolveResource(core::ref<Texture> const& src, core::ref<Texture> const& dst) -> void;
        auto resolveSubresource(core::ref<Texture> const& src, uint32_t srcSubresource, core::ref<Texture> const& dst, uint32_t dstSubresource) -> void;

    private:
        auto applyState(
            std::vector<Viewport> const& viewports,
            std::vector<Scissor> const& scissors,
            core::ref<VertexArrayObject> const& vao = nullptr
        ) -> void;
        CommandContext* mp_context{nullptr};
        std::unique_ptr<BlitContext> mp_blitContext;

        // StateBindFlags m_bindFlags{StateBindFlags::All};
        GraphicsPipeline* mp_lastBoundPipeline{nullptr};
        ProgramVariables* mp_lastBoundGraphicsVars{nullptr};

        ColorTargets m_colorTargets{};
        DepthStencilTarget m_depthStencilTarget{};
        std::vector<Viewport> m_viewports{};
        std::vector<Scissor> m_scissors{};
        core::ref<VertexArrayObject> m_lastBoundVao{nullptr};
        // Reset rhi RenderState when render state is dirty.
        bool m_renderStateDirty{};

        std::vector<rhi::RenderPassColorAttachment> m_gfxColorAttachments;
        rhi::RenderPassDepthStencilAttachment m_gfxDepthStencilAttachment;
        bool m_hasDepthStencil{false};
    };

    class ComputePassEncoder final : public PassEncoderBase<rhi::IComputePassEncoder>, public core::Object
    {
        APRIL_OBJECT(ComputePassEncoder)
    public:
        using PassEncoderBase::end;
        friend class CommandContext;

        ComputePassEncoder(CommandContext* pCtx, rhi::IComputePassEncoder* pEncoder) : PassEncoderBase(pEncoder), mp_context(pCtx) {}

        auto bindPipeline(ComputePipeline* pipeline, ProgramVariables* vars) -> void;

        auto dispatch(uint3 const& dispatchSize) -> void;

        auto dispatchIndirect(Buffer const* argBuffer, uint64_t argBufferOffset) -> void;

        auto clearUAV(UnorderedAccessView const* uav, float4 const& value) -> void;
        auto clearUAV(UnorderedAccessView const* uav, uint4 const& value) -> void;
        /**
        * Clear a structured buffer's UAV counter
        * @param[in] pBuffer Structured Buffer containing UAV counter
        * @param[in] value Value to clear counter to
        */
        auto clearUAVCounter(core::ref<Buffer> const& buffer, uint32_t value) -> void;

    private:
        CommandContext* mp_context{nullptr};
        ComputePipeline* m_lastBoundComputePipeline{nullptr};
        ProgramVariables* m_lastBoundComputeVars{nullptr};
    };

    class RayTracingPassEncoder final : public PassEncoderBase<rhi::IRayTracingPassEncoder>, public core::Object
    {
        APRIL_OBJECT(RayTracingPassEncoder)
    public:
        using PassEncoderBase::end;
        friend class CommandContext;

        RayTracingPassEncoder(CommandContext* pCtx, rhi::IRayTracingPassEncoder* pEncoder) : PassEncoderBase(pEncoder), mp_context(pCtx) {}

        auto bindPipeline(RayTracingPipeline* pipeline, RtProgramVariables* vars) -> void;

        auto raytrace(uint32_t width, uint32_t height, uint32_t depth) -> void;

        auto buildAccelerationStructure(
            RtAccelerationStructure::BuildDesc const& desc,
            uint32_t postBuildInfoCount,
            RtAccelerationStructurePostBuildInfoDesc* postBuildInfoDescs
        ) -> void;

        auto copyAccelerationStructure(RtAccelerationStructure* dest, RtAccelerationStructure* source, RtAccelerationStructureCopyMode mode) -> void;

    private:
        CommandContext* mp_context{nullptr};
        RayTracingPipeline* m_lastBoundPipeline{nullptr};
        RtProgramVariables* m_lastBoundRtVars{nullptr};
    };

    class CommandContext final : public core::Object
    {
        APRIL_OBJECT(CommandContext)
    public:
        CommandContext(Device* device, rhi::ICommandQueue* queue);
        ~CommandContext();

        auto beginFrame() -> void;
        auto endFrame() -> void;

        auto beginRenderPass(
            ColorTargets const& pColorTargets,
            DepthStencilTarget const& pDepthStencilTarget = {}
        ) -> core::ref<RenderPassEncoder>;
        auto beginComputePass() -> core::ref<ComputePassEncoder>;
        auto beginRayTracingPass() -> core::ref<RayTracingPassEncoder>;

        class ReadTextureTask
        {
        public:
            using SharedPtr = std::shared_ptr<ReadTextureTask>;
            static auto create(CommandContext* pCtx, Texture const* pTexture, uint32_t subresourceIndex) -> SharedPtr;
            auto getData(void* pData, size_t size) const -> void;
            auto getData() const -> std::vector<uint8_t>;

        private:
            ReadTextureTask() = default;
            core::ref<Fence> m_fence{};
            core::ref<Buffer> m_buffer{};
            CommandContext* m_context{nullptr};
            uint32_t m_rowCount{0};
            uint32_t m_rowSize{0};
            uint32_t m_actualRowSize{0};
            uint32_t m_depth{0};
        };

        auto getDevice() const -> core::ref<Device>;
        auto getProfiler() const -> Profiler*;

        /**
        * Flush the command list. This doesn't reset the command allocator, just submits the commands
        * @param[in] wait If true, will block execution until the GPU finished processing the commands
        */
        auto submit(bool wait = false) -> void;

        auto hasPendingCommands() const -> bool { return m_commandsPending; }
        auto setPendingCommands(bool commandsPending) -> void { m_commandsPending = commandsPending; }

        /**
        * Signal a fence.
        * @param pFence The fence to signal.
        * @param value The value to signal. If Fence::kAuto, the signaled value will be auto-incremented.
        * @return Returns the signaled value.
        */
        auto signal(Fence* fence, uint64_t value = Fence::kAuto) -> uint64_t;

        /**
        * Wait for a fence to be signaled on the device.
        * Queues a device-side wait and returns immediately.
        * The device will wait until the fence reaches or exceeds the specified value.
        * @param pFence The fence to wait for.
        * @param value The value to wait for. If Fence::kAuto, wait for the last signaled value.
        */
        auto wait(Fence* fence, uint64_t value = Fence::kAuto) -> void;

        // Because can not insert memory barriers inside render passes, we do it here.
        auto clearRtv(RenderTargetView const* rtv, float4 const& color) -> void;
        auto clearDsv(DepthStencilView const* dsv, float depth, uint8_t stencil, bool clearDepth = true, bool clearStencil = true) -> void;
        auto clearTexture(Texture* texture, float4 const& clearColor = {0, 0, 0, 1}) -> void;
        /**
        * Insert a resource barrier
        * if pViewInfo is nullptr, will transition the entire resource. Otherwise, it will only transition the subresource in the view
        * @return true if a barrier commands were recorded for the entire resource-view, otherwise false (for example, when the current
        * resource state is the same as the new state or when only some subresources were transitioned)
        */
        auto resourceBarrier(Resource const* resource, Resource::State newState, ResourceViewInfo const* viewInfo = nullptr) -> bool;
        auto uavBarrier(Resource const* resource) -> void;

        // auto copyResource(Resource const* dst, Resource const* src) -> void;
        auto copyBuffer(Buffer const* dst, Buffer const* src) -> void;
        auto copyTexture(Texture* const dst, Texture const* src) -> void;
        auto copySubresource(Texture const* dst, uint32_t dstSubresourceIdx, Texture const* src, uint32_t srcSubresourceIdx) -> void;
        auto copyBufferRegion(Buffer const* dst, uint64_t dstOffset, Buffer const* src, uint64_t srcOffset, uint64_t numBytes) -> void;

        auto copySubresourceRegion(
            Texture const* dst,
            uint32_t dstSubresource,
            Texture const* src,
            uint32_t srcSubresource,
            uint3 const& dstOffset = uint3(0),
            uint3 const& srcOffset = uint3(0),
            uint3 const& size = uint3(kUintMax)
        ) -> void;

        auto updateSubresourceData(
            Texture const* dst,
            uint32_t subresource,
            void const* pData,
            uint3 const& offset = uint3(0),
            uint3 const& size = uint3(kUintMax)
        ) -> void;

        auto updateTextureData(Texture const* texture, void const* data) -> void;
        auto updateBuffer(Buffer const* buffer, void const* data, size_t offset = 0, size_t numBytes = 0) -> void;
        auto readBuffer(Buffer const* buffer, void* data, size_t offset = 0, size_t numBytes = 0) -> void;

        template<typename T>
        auto readBuffer(Buffer const* buffer, size_t firstElement = 0, size_t elementCount = 0) -> std::vector<T>
        {
            if (elementCount == 0)
                elementCount = buffer->getSize() / sizeof(T);

            size_t offset = firstElement * sizeof(T);
            size_t numBytes = elementCount * sizeof(T);

            std::vector<T> result(elementCount);
            readBuffer(buffer, result.data(), offset, numBytes);
            return result;
        }

        auto readTextureSubresource(Texture const* texture, uint32_t subresourceIndex) -> std::vector<uint8_t>;
        auto asyncReadTextureSubresource(Texture const* texture, uint32_t subresourceIndex) -> ReadTextureTask::SharedPtr;

        auto getLowLevelData() const -> LowLevelContextData* { return mp_lowLevelData.get(); }

        auto bindDescriptorHeaps() -> void;
        auto bindCustomGPUDescriptorPool() -> void;
        auto unbindCustomGPUDescriptorPool() -> void;

    private:
        auto textureBarrier(Texture const* texture, Resource::State newState) -> bool;
        auto bufferBarrier(Buffer const* buffer, Resource::State newState) -> bool;
        auto subresourceBarriers(Texture const* texture, Resource::State newState, ResourceViewInfo const* viewInfo) -> bool;
        auto apiSubresourceBarrier(
            Texture const* texture,
            Resource::State newState,
            Resource::State oldState,
            uint32_t arraySlice,
            uint32_t mipLevel
        ) -> void;

        auto updateTextureSubresources(
            Texture const* texture,
            uint32_t firstSubresource,
            uint32_t subresourceCount,
            void const* pData,
            uint3 const& offset = uint3(0),
            uint3 const& size = uint3(kUintMax)
        ) -> void;

        Device* mp_device{nullptr};
        std::unique_ptr<LowLevelContextData> mp_lowLevelData;
        bool m_commandsPending{false};
    };
}

namespace april::graphics
{
    template <class TEncoder>
    auto PassEncoderBase<TEncoder>::pushDebugGroup(std::string_view name, float4 color) -> void
    {
        m_encoder->pushDebugGroup(name.data(), color);
    }

    template <class TEncoder>
    auto PassEncoderBase<TEncoder>::popDebugGroup() -> void
    {
        m_encoder->popDebugGroup();
    }

    template <class TEncoder>
    auto PassEncoderBase<TEncoder>::insertDebugMarker(std::string_view name, float4 color) -> void
    {
        m_encoder->insertDebugMarker(name.data(), color);
    }

    template <class TEncoder>
    auto PassEncoderBase<TEncoder>::writeTimestamp(QueryHeap* pHeap, uint32_t index) -> void
    {
        m_encoder->writeTimestamp(pHeap, index);
    }

    template <class TEncoder>
    auto PassEncoderBase<TEncoder>::end() -> void
    {
        m_encoder->end();
    }
}
