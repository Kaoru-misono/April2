#pragma once

#include "foundation/object.hpp"
#include "fwd.hpp"
#include "blit-context.hpp"
#include "graphics-pipeline.hpp"
#include "compute-pipeline.hpp"
#include "ray-tracing-pipeline.hpp"
#include "program/program.hpp"
#include "resource.hpp"
#include "rhi/buffer.hpp"
#include "texture.hpp"
#include "sampler.hpp"
#include "ray-tracing-acceleration-structure.hpp"
#include "render-target.hpp"
#include "program/program-variables.hpp"
#include "fence.hpp"

#include <core/math/type.hpp>
#include <slang-rhi.h>
#include <slang-com-ptr.h>
#include <string_view>

namespace april::core { class Profiler; }

namespace april::graphics
{
    class CommandContext; // Forward declaration

    template <class TEncoder>
    class PassEncoderBase {
    public:
        PassEncoderBase() = default;
        explicit PassEncoderBase(TEncoder* pEncoder)
            : m_encoder(pEncoder) {}

        auto pushDebugGroup(std::string_view name, float4 color = {1.0, 1.0f, 1.0f, 1.0f}) -> void;

        auto popDebugGroup() -> void;

        auto insertDebugMarker(std::string_view name, float4 color = {1.0, 1.0f, 1.0f, 1.0f}) -> void;

        auto writeTimestamp(QueryHeap* pHeap, uint32_t index) -> void;
        auto writeTimestamp(core::ref<QueryHeap> const& pHeap, uint32_t index) -> void { writeTimestamp(pHeap.get(), index); }

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

        auto drawIndirect(
            uint32_t maxCommandCount,
            core::ref<Buffer> const& argBuffer,
            uint64_t argBufferOffset,
            core::ref<Buffer> const& countBuffer,
            uint64_t countBufferOffset
        ) -> void
        {
            drawIndirect(
                maxCommandCount,
                argBuffer.get(),
                argBufferOffset,
                countBuffer.get(),
                countBufferOffset
            );
        }

        auto drawIndexedIndirect(
            uint32_t maxCommandCount,
            core::ref<Buffer> const& argBuffer,
            uint64_t argBufferOffset,
            core::ref<Buffer> const& countBuffer,
            uint64_t countBufferOffset
        ) -> void
        {
            drawIndexedIndirect(
                maxCommandCount,
                argBuffer.get(),
                argBufferOffset,
                countBuffer.get(),
                countBufferOffset
            );
        }

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

        // Helpers for use core::ref
        auto bindPipeline(core::ref<ComputePipeline> const& pipeline, core::ref<ProgramVariables> const& vars) -> void { bindPipeline(pipeline.get(), vars.get()); }
        auto dispatchIndirect(core::ref<Buffer> const& argBuffer, uint64_t argBufferOffset) -> void { dispatchIndirect(argBuffer.get(), argBufferOffset); }

        auto clearUAV(core::ref<UnorderedAccessView> const& uav, float4 const& value) -> void { clearUAV(uav.get(), value); }
        auto clearUAV(core::ref<UnorderedAccessView> const& uav, uint4 const& value) -> void { clearUAV(uav.get(), value); }


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

    struct SubmissionPayload
    {
        rhi::ComPtr<rhi::ICommandBuffer> commandBuffer{nullptr};
        std::vector<std::pair<rhi::IFence*, uint64_t>> waitFences;
        std::vector<std::pair<rhi::IFence*, uint64_t>> signalFences;
    };

    class CommandContext final : public core::Object
    {
        APRIL_OBJECT(CommandContext)
    public:
        CommandContext(Device* device, rhi::ICommandQueue* queue);
        ~CommandContext();

        // --- Pass Creators ---
        auto beginRenderPass(
            ColorTargets const& pColorTargets,
            DepthStencilTarget const& pDepthStencilTarget = {}
        ) -> core::ref<RenderPassEncoder>;
        auto beginComputePass() -> core::ref<ComputePassEncoder>;
        auto beginRayTracingPass() -> core::ref<RayTracingPassEncoder>;

        // --- Helpers ---
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

        // --- Submission & Synchronization ---

        /**
         * Finishes recording and returns the submission payload.
         * This resets the context for new recording.
         * Call this from worker threads in a multi-threaded renderer.
         */
        auto finish() -> SubmissionPayload;

        /**
         * Convenience wrapper: Calls finish() and immediately submits to the queue.
         * Use this for single-threaded rendering or immediate resource operations.
         * @param wait If true, CPU will wait for GPU completion (idle wait).
         */
        auto submit(bool wait = false) -> void;

        auto hasPendingCommands() const -> bool { return m_commandsPending; }
        auto setPendingCommands(bool commandsPending) -> void { m_commandsPending = commandsPending; }

        /**
         * Enqueue a fence to signal when this batch completes.
         */
        auto enqueueSignal(Fence* fence, uint64_t value = Fence::kAuto) -> void;

        /**
         * Enqueue a fence to wait before this batch starts.
         */
        auto enqueueWait(Fence* fence) -> void;

        /**
         * CPU-side wait. Warning: This blocks the calling thread.
         */
        auto wait(Fence* fence, uint64_t value = Fence::kAuto) -> void;

        // --- Resource Commands ---
        auto clearRtv(RenderTargetView const* rtv, float4 const& color) -> void;
        auto clearDsv(DepthStencilView const* dsv, float depth, uint8_t stencil, bool clearDepth = true, bool clearStencil = true) -> void;
        auto clearTexture(Texture* texture, float4 const& clearColor = {0, 0, 0, 1}) -> void;
        auto clearBuffer(Buffer const* buffer) -> void;

        auto resourceBarrier(Resource const* resource, Resource::State newState, ResourceViewInfo const* viewInfo = nullptr) -> bool;
        auto uavBarrier(Resource const* resource) -> void;
        auto bufferBarrier(Buffer const* buffer, Resource::State newState) -> bool;

        auto copyBuffer(Buffer const* dst, Buffer const* src) -> void;
        auto copyTexture(Texture const* dst, Texture const* src) -> void;
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

        template <typename T>
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

        // --- Debug & Profiling ---
        auto pushDebugGroup(std::string_view name, float4 color = {1.0, 1.0f, 1.0f, 1.0f}) -> void;
        auto popDebugGroup() -> void;
        auto insertDebugMarker(std::string_view name, float4 color = {1.0, 1.0f, 1.0f, 1.0f}) -> void;

        auto writeTimestamp(QueryHeap* pHeap, uint32_t index) -> void;
        auto resolveQuery(QueryHeap* pHeap, uint32_t index, uint32_t count, Buffer const* buffer, uint64_t offset) -> void;

        // --- Accessors ---
        auto getDevice() const -> core::ref<Device>;
        auto getProfiler() const -> core::Profiler*; // Assuming Device holds Profiler
        auto getGfxCommandQueue() const -> rhi::ICommandQueue* { return mp_gfxCommandQueue; }
        auto getGfxCommandEncoder() const -> rhi::ICommandEncoder* { return m_gfxEncoder; }

        auto getCommandQueueNativeHandle() const -> rhi::NativeHandle;

        auto bindDescriptorHeaps() -> void;
        auto bindCustomGPUDescriptorPool() -> void;
        auto unbindCustomGPUDescriptorPool() -> void;

        // Helper for use core::ref<T>
        auto clearRtv(core::ref<RenderTargetView> const& rtv, float4 const& color) -> void { clearRtv(rtv.get(), color); }
        auto clearDsv(core::ref<DepthStencilView> const& dsv, float depth, uint8_t stencil, bool clearDepth = true, bool clearStencil = true) -> void { clearDsv(dsv.get(), depth, stencil, clearDepth, clearStencil); }
        auto clearTexture(core::ref<Texture> const& texture, float4 const& clearColor = {0, 0, 0, 1}) -> void { clearTexture(texture.get(), clearColor); }
        auto clearBuffer(core::ref<Buffer> const& buffer) -> void { clearBuffer(buffer.get()); }

        auto resourceBarrier(core::ref<Resource> const& resource, Resource::State newState, ResourceViewInfo const* viewInfo = nullptr) -> bool { return resourceBarrier(resource.get(), newState, viewInfo); }
        auto uavBarrier(core::ref<Resource> const& resource) -> void { uavBarrier(resource.get()); }
        auto bufferBarrier(core::ref<Buffer> const& buffer, Resource::State newState) -> bool { return bufferBarrier(buffer.get(), newState); }

        auto copyBuffer(core::ref<Buffer> const& dst, core::ref<Buffer> const& src) -> void { copyBuffer(dst.get(), src.get()); }
        auto copyTexture(core::ref<Texture> const& dst, core::ref<Texture> const& src) -> void { copyTexture(dst.get(), src.get()); }
        auto copySubresource(core::ref<Texture> const& dst, uint32_t dstSubresourceIdx, core::ref<Texture> const& src, uint32_t srcSubresourceIdx) -> void { copySubresource(dst.get(), dstSubresourceIdx, src.get(), srcSubresourceIdx); }
        auto copyBufferRegion(core::ref<Buffer> const& dst, uint64_t dstOffset, core::ref<Buffer> const& src, uint64_t srcOffset, uint64_t numBytes) -> void { copyBufferRegion(dst.get(), dstOffset, src.get(), srcOffset, numBytes); }

        auto copySubresourceRegion(
            core::ref<Texture> const& dst,
            uint32_t dstSubresource,
            core::ref<Texture> const& src,
            uint32_t srcSubresource,
            uint3 const& dstOffset = uint3(0),
            uint3 const& srcOffset = uint3(0),
            uint3 const& size = uint3(kUintMax)
        ) -> void
        {
            copySubresourceRegion(
                dst.get(),
                dstSubresource,
                src.get(),
                srcSubresource,
                dstOffset,
                srcOffset,
                size
            );
        }

        auto updateSubresourceData(
            core::ref<Texture> const& dst,
            uint32_t subresource,
            void const* pData,
            uint3 const& offset = uint3(0),
            uint3 const& size = uint3(kUintMax)
        ) -> void
        {
            updateSubresourceData(
                dst.get(),
                subresource,
                pData,
                offset,
                size
            );
        }

        auto updateTextureData(core::ref<Texture> const& texture, void const* data) -> void { updateTextureData(texture.get(), data); }
        auto updateBuffer(core::ref<Buffer> const& buffer, void const* data, size_t offset = 0, size_t numBytes = 0) -> void { updateBuffer(buffer.get(), data, offset, numBytes); }
        auto readBuffer(core::ref<Buffer> const& buffer, void* data, size_t offset = 0, size_t numBytes = 0) -> void { readBuffer(buffer.get(), data, offset, numBytes); }

        template <typename T>
        auto readBuffer(core::ref<Buffer> const& buffer, size_t firstElement = 0, size_t elementCount = 0) -> std::vector<T>
        {
            return readBuffer<T>(buffer.get(), firstElement, elementCount);
        }

        auto readTextureSubresource(core::ref<Texture> texture, uint32_t subresourceIndex) -> std::vector<uint8_t> { return readTextureSubresource(texture.get(), subresourceIndex); }
        auto asyncReadTextureSubresource(core::ref<Texture> texture, uint32_t subresourceIndex) -> ReadTextureTask::SharedPtr { return asyncReadTextureSubresource(texture.get(), subresourceIndex); };

        auto writeTimestamp(core::ref<QueryHeap> const& pHeap, uint32_t index) -> void { writeTimestamp(pHeap.get(), index); }
        auto resolveQuery(core::ref<QueryHeap> const& pHeap, uint32_t index, uint32_t count, core::ref<Buffer> const& buffer, uint64_t offset) -> void { resolveQuery(pHeap.get(), index, count, buffer.get(), offset); }

    private:
        auto textureBarrier(Texture const* texture, Resource::State newState) -> bool;
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
        rhi::ICommandQueue* mp_gfxCommandQueue{};
        rhi::ComPtr<rhi::ICommandEncoder> m_gfxEncoder{};

        // Pending synchronization primitives
        std::vector<std::pair<rhi::IFence*, uint64_t>> m_pendingWaitFences{};
        std::vector<std::pair<rhi::IFence*, uint64_t>> m_pendingSignalFences{};

        bool m_commandsPending{false};
    };
}

namespace april::graphics
{
    template <class TEncoder>
    auto PassEncoderBase<TEncoder>::pushDebugGroup(std::string_view name, float4 color) -> void
    {
        auto markerColor = rhi::MarkerColor{color.r, color.g, color.b};
        m_encoder->pushDebugGroup(name.data(), markerColor);
    }

    template <class TEncoder>
    auto PassEncoderBase<TEncoder>::popDebugGroup() -> void
    {
        m_encoder->popDebugGroup();
    }

    template <class TEncoder>
    auto PassEncoderBase<TEncoder>::insertDebugMarker(std::string_view name, float4 color) -> void
    {
        m_encoder->insertDebugMarker(name.data(), rhi::MarkerColor{color.r, color.g, color.b});
    }

    template <class TEncoder>
    auto PassEncoderBase<TEncoder>::writeTimestamp(QueryHeap* pHeap, uint32_t index) -> void
    {
        m_encoder->writeTimestamp(pHeap->getGfxQueryPool(), index);
    }

    template <class TEncoder>
    auto PassEncoderBase<TEncoder>::end() -> void
    {
        m_encoder->end();
    }
}
