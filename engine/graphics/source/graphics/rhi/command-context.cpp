// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "command-context.hpp"
#include "render-device.hpp"
#include "graphics-pipeline.hpp"

#include "compute-pipeline.hpp"
#include "ray-tracing-pipeline.hpp"
#include "rhi-tools.hpp"
#include "rhi/rhi-tools.hpp"
#include "rhi/vertex-array-object.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "fence.hpp"
#include "gpu-memory-heap.hpp"
#include "graphics/program/program-variables.hpp"

#include <core/error/assert.hpp>
#include <core/tools/alignment.hpp>
#include <core/tools/enum-flags.hpp>
#include <core/math/math.hpp>
#include <slang-rhi.h>
#include <ranges>

namespace april::graphics
{
    // ReadTextureTask Implementation

    auto CommandContext::ReadTextureTask::create(CommandContext* pCtx, Texture const* pTexture, uint32_t subresourceIndex) -> SharedPtr
    {
        SharedPtr pThis = SharedPtr(new ReadTextureTask);
        pThis->m_context = pCtx;

        rhi::ITexture* srcTexture = pTexture->getGfxTextureResource();
        rhi::FormatInfo formatInfo = rhi::getFormatInfo(srcTexture->getDesc().format);

        auto mipLevel = pTexture->getSubresourceMipLevel(subresourceIndex);
        pThis->m_actualRowSize = uint32_t((pTexture->getWidth(mipLevel) + formatInfo.blockWidth - 1) / formatInfo.blockWidth * formatInfo.blockSizeInBytes);

        size_t rowAlignment = 1;
        pCtx->mp_device->getGfxDevice()->getTextureRowAlignment(&rowAlignment);
        pThis->m_rowSize = align_up(pThis->m_actualRowSize, static_cast<size_t>(rowAlignment));
        uint64_t rowCount = (pTexture->getHeight(mipLevel) + formatInfo.blockHeight - 1) / formatInfo.blockHeight;
        uint64_t size = pTexture->getDepth(mipLevel) * rowCount * pThis->m_rowSize;

        pThis->m_buffer = pCtx->getDevice()->createBuffer(size, BufferUsage::None, MemoryType::ReadBack, nullptr);

        pCtx->resourceBarrier(pTexture, Resource::State::CopySource);
        auto resourceEncoder = pCtx->getGfxCommandEncoder();

        resourceEncoder->copyTextureToBuffer(
            pThis->m_buffer->getGfxBufferResource(),
            0,
            size,
            pThis->m_rowSize,
            srcTexture,
            pTexture->getSubresourceArraySlice(subresourceIndex),
            mipLevel,
            {0, 0, 0},
            {pTexture->getWidth(mipLevel), pTexture->getHeight(mipLevel), pTexture->getDepth(mipLevel)}
        );
        pCtx->setPendingCommands(true);

        pThis->m_fence = pCtx->getDevice()->createFence();
        pThis->m_fence->breakStrongReferenceToDevice();

        // FIX: Optimized fence handling.
        // Instead of submit() then signal() (which causes two batches),
        // we enqueue the signal and submit once.
        pCtx->enqueueSignal(pThis->m_fence.get());
        pCtx->submit(false);

        pThis->m_rowCount = (uint32_t)rowCount;
        pThis->m_depth = pTexture->getDepth(mipLevel);

        return pThis;
    }

    auto CommandContext::ReadTextureTask::getData(void* pData, size_t size) const -> void
    {
        AP_ASSERT(size == size_t(m_rowCount) * m_actualRowSize * m_depth);

        // This wait is a CPU blocking wait, which is acceptable for ReadBack tasks
        m_fence->wait();

        uint8_t* pDst = reinterpret_cast<uint8_t*>(pData);
        uint8_t const* pSrc = reinterpret_cast<uint8_t const*>(m_buffer->map());

        for (uint32_t z = 0; z < m_depth; z++)
        {
            uint8_t const* pSrcZ = pSrc + z * (size_t)m_rowSize * m_rowCount;
            uint8_t* pDstZ = pDst + z * (size_t)m_actualRowSize * m_rowCount;
            for (uint32_t y = 0; y < m_rowCount; y++)
            {
                uint8_t const* pSrcY = pSrcZ + y * (size_t)m_rowSize;
                uint8_t* pDstY = pDstZ + y * (size_t)m_actualRowSize;
                std::memcpy(pDstY, pSrcY, m_actualRowSize);
            }
        }

        m_buffer->unmap();
    }

    auto CommandContext::ReadTextureTask::getData() const -> std::vector<uint8_t>
    {
        std::vector<uint8_t> result(size_t(m_rowCount) * m_actualRowSize * m_depth);
        getData(result.data(), result.size());
        return result;
    }

    CommandContext::CommandContext(Device* device, rhi::ICommandQueue* queue)
        : mp_device(device), mp_gfxCommandQueue(queue)
    {
        m_gfxEncoder = mp_gfxCommandQueue->createCommandEncoder();
    }

    CommandContext::~CommandContext() = default;

    auto CommandContext::getCommandQueueNativeHandle() const -> rhi::NativeHandle
    {
        rhi::NativeHandle gfxNativeHandle = {};
        checkResult(mp_gfxCommandQueue->getNativeHandle(&gfxNativeHandle), "Failed to get command queue native handle");
        return gfxNativeHandle;
    }

    auto CommandContext::finish() -> SubmissionPayload
    {
        SubmissionPayload payload;

        if (m_commandsPending)
        {
            // Close the Slang/RHI encoder to produce the command buffer
            checkResult(m_gfxEncoder->finish(payload.commandBuffer.writeRef()), "Failed to close command buffer");
        }
        else
        {
            payload.commandBuffer = nullptr;
        }

        // Move pending fences to payload
        // This transfers ownership of the pending lists to the caller
        payload.waitFences = std::move(m_pendingWaitFences);
        payload.signalFences = std::move(m_pendingSignalFences);

        // Reset local state for next frame/pass
        // Note: m_pendingWaitFences/signalFences are already empty after std::move
        m_commandsPending = false;

        // Prepare new encoder for next use
        m_gfxEncoder = mp_gfxCommandQueue->createCommandEncoder();

        return payload;
    }

    // [Refactored] submit() is now a convenience wrapper around finish()
    auto CommandContext::submit(bool wait) -> void
    {
        // 1. Finish recording and get payload
        auto payload = finish();

        // 2. If there's nothing to execute and no fences to signal/wait, skip
        if (!payload.commandBuffer && payload.waitFences.empty() && payload.signalFences.empty())
        {
            return;
        }

        // 3. Prepare RHI submission descriptor
        // Convert the vectors to raw pointer arrays required by RHI C-API
        auto waitFences = payload.waitFences | std::views::keys | std::ranges::to<std::vector>();
        auto waitValues = payload.waitFences | std::views::values | std::ranges::to<std::vector>();

        auto signalFences = payload.signalFences | std::views::keys | std::ranges::to<std::vector>();
        auto signalValues = payload.signalFences | std::views::values | std::ranges::to<std::vector>();

        auto commandBuffer = payload.commandBuffer.get();
        rhi::SubmitDesc submitDesc = {};
        submitDesc.commandBuffers = payload.commandBuffer ? &commandBuffer : nullptr;
        submitDesc.commandBufferCount = payload.commandBuffer ? 1 : 0;

        submitDesc.waitFences = waitFences.data();
        submitDesc.waitFenceValues = waitValues.data();
        submitDesc.waitFenceCount = (uint32_t)waitFences.size();

        submitDesc.signalFences = signalFences.data();
        submitDesc.signalFenceValues = signalValues.data();
        submitDesc.signalFenceCount = (uint32_t)signalFences.size();

        // 4. Submit to Queue (The actual GPU kick)
        checkResult(mp_gfxCommandQueue->submit(submitDesc), "Failed to submit command buffer");

        // 5. Handle bind descriptors (usually needed after reset/submit in some APIs)
        bindDescriptorHeaps();

        // 6. Optional CPU wait (Blocking)
        if (wait)
        {
            mp_gfxCommandQueue->waitOnHost();
        }
    }

    auto CommandContext::enqueueSignal(Fence* fence, uint64_t value) -> void
    {
        AP_ASSERT(fence, "'fence' must not be null");
        uint64_t signalValue = fence->updateSignaledValue(value);
        m_pendingSignalFences.emplace_back(fence->getGfxFence(), signalValue);
    }

    auto CommandContext::enqueueWait(Fence* fence) -> void
    {
        AP_ASSERT(fence, "'fence' must not be null");
        m_pendingWaitFences.emplace_back(fence->getGfxFence(), fence->getSignaledValue());
    }

    auto CommandContext::wait(Fence* fence, uint64_t value) -> void
    {
        AP_ASSERT(fence, "'fence' must not be null");
        uint64_t waitValue = value == Fence::kAuto ? fence->getSignaledValue() : value;
        rhi::IFence* fences[] = {fence->getGfxFence()};
        uint64_t waitValues[] = {waitValue};

        checkResult(mp_device->getGfxDevice()->waitForFences(1, fences, waitValues, true, UINTMAX_MAX), "Failed to wait for fence");
    }

    auto CommandContext::getDevice() const -> core::ref<Device>
    {
        return core::ref<Device>(mp_device);
    }

    auto CommandContext::bindDescriptorHeaps() -> void {}

    auto CommandContext::bindCustomGPUDescriptorPool() -> void
    {
#if defined(_WIN32)
        // D3D12 specific logic
#endif
    }

    auto CommandContext::unbindCustomGPUDescriptorPool() -> void
    {
#if defined(_WIN32)
        // D3D12 specific logic
#endif
    }

    // RenderPassEncoder Implementation

    RenderPassEncoder::RenderPassEncoder(
        CommandContext* pCtx,
        ColorTargets const& pColorTargets,
        DepthStencilTarget const& pDepthStencilTarget
    )
        : PassEncoderBase<rhi::IRenderPassEncoder>()
        , mp_context(pCtx)
        , m_colorTargets(pColorTargets)
        , m_depthStencilTarget(pDepthStencilTarget)
    {
        static auto getLoadOp = [](LoadOp op) -> rhi::LoadOp {
            switch (op)
            {
                case LoadOp::Load: return rhi::LoadOp::Load;
                case LoadOp::Clear: return rhi::LoadOp::Clear;
                case LoadOp::DontCare: return rhi::LoadOp::DontCare;
                default: return rhi::LoadOp::DontCare;
            }
        };

        static auto getStoreOp = [](StoreOp op) -> rhi::StoreOp {
            switch (op)
            {
                case StoreOp::Store: return rhi::StoreOp::Store;
                case StoreOp::DontCare: return rhi::StoreOp::DontCare;
                default: return rhi::StoreOp::DontCare;
            }
        };

        for (auto& colorTarget: pColorTargets)
        {
            AP_ASSERT(colorTarget.m_colorTargetView, "Color target have a invalid view.");

            rhi::RenderPassColorAttachment att = {};
            att.view = colorTarget.getGfxTextureView();
            att.loadOp = getLoadOp(colorTarget.loadOp);
            att.storeOp = getStoreOp(colorTarget.storeOp);
            att.clearValue[0] = colorTarget.clearColor.x;
            att.clearValue[1] = colorTarget.clearColor.y;
            att.clearValue[2] = colorTarget.clearColor.z;
            att.clearValue[3] = colorTarget.clearColor.w;
            m_gfxColorAttachments.push_back(att);
        }

        if (pDepthStencilTarget)
        {
            m_gfxDepthStencilAttachment.view = pDepthStencilTarget.getGfxTextureView();
            m_gfxDepthStencilAttachment.depthLoadOp = getLoadOp(pDepthStencilTarget.depthLoadOp);
            m_gfxDepthStencilAttachment.depthStoreOp = getStoreOp(pDepthStencilTarget.depthStoreOp);
            m_gfxDepthStencilAttachment.stencilLoadOp = getLoadOp(pDepthStencilTarget.stencilLoadOp);
            m_gfxDepthStencilAttachment.stencilStoreOp = getStoreOp(pDepthStencilTarget.stencilStoreOp);
            m_hasDepthStencil = true;
        }
    }

    auto RenderPassEncoder::bindPipeline(GraphicsPipeline* pipeline, ProgramVariables* vars) -> void
    {
        mp_lastBoundPipeline = pipeline;
        mp_lastBoundGraphicsVars = vars;

        if (vars)
        {
            vars->prepareDescriptorSets(mp_context);
        }

        m_encoder->bindPipeline(pipeline->getGfxPipeline(), vars ? vars->getShaderObject() : nullptr);
    }

    auto RenderPassEncoder::applyState(
        std::vector<Viewport> const& viewports,
        std::vector<Scissor> const& scissors,
        core::ref<VertexArrayObject> const& vao
    ) -> void
    {
        if (!m_renderStateDirty) return;

        rhi::RenderState renderState = {};
        renderState.viewportCount = std::min((uint32_t)viewports.size(), 16u);
        for (uint32_t i = 0; i < renderState.viewportCount; ++i)
        {
            auto& vp = viewports[i];
            renderState.viewports[i].originX = vp.x;
            renderState.viewports[i].originY = vp.y;
            renderState.viewports[i].extentX = vp.width;
            renderState.viewports[i].extentY = vp.height;
            renderState.viewports[i].minZ = vp.minDepth;
            renderState.viewports[i].maxZ = vp.maxDepth;
        }

        renderState.scissorRectCount = std::min((uint32_t)scissors.size(), 16u);
        for (uint32_t i = 0; i < renderState.scissorRectCount; ++i)
        {
            auto& sc = scissors[i];
            renderState.scissorRects[i].minX = (int32_t)sc.offsetX;
            renderState.scissorRects[i].minY = (int32_t)sc.offsetY;
            renderState.scissorRects[i].maxX = (int32_t)(sc.offsetX + sc.extentX);
            renderState.scissorRects[i].maxY = (int32_t)(sc.offsetY + sc.extentY);
        }

        if (vao)
        {
            renderState.vertexBufferCount = vao ? (uint32_t)vao->getVertexBuffersCount() : 0;
            for (uint32_t i = 0; i < renderState.vertexBufferCount; ++i)
            {
                auto vb = vao->getVertexBuffer(i);
                renderState.vertexBuffers[i].buffer = vb->getGfxBufferResource();
                // TODO: Check offset.
                renderState.vertexBuffers[i].offset = 0;
            }
            if (auto indexBuffer = vao->getIndexBuffer())
            {
                renderState.indexBuffer.buffer = indexBuffer->getGfxBufferResource();
                renderState.indexBuffer.offset = 0;
                renderState.indexFormat = vao->getIndexBufferFormat() == ResourceFormat::R16Uint ? rhi::IndexFormat::Uint16 : rhi::IndexFormat::Uint32;
            }
        }

        m_encoder->setRenderState(renderState);
        m_renderStateDirty = false;
    }

    auto RenderPassEncoder::setViewport(uint32_t index, Viewport const& vp) -> void
    {
        if (index >= m_viewports.size()) m_viewports.resize(index + 1);
        m_viewports[index] = vp;

        m_renderStateDirty = true;
    }

    auto RenderPassEncoder::setScissor(uint32_t index, Scissor const& sc) -> void
    {
        if (index >= m_scissors.size()) m_scissors.resize(index + 1);
        m_scissors[index] = sc;

        m_renderStateDirty = true;
    }

    auto RenderPassEncoder::setVao(core::ref<VertexArrayObject> const& vao) -> void
    {
        m_lastBoundVao = vao;

        m_renderStateDirty = true;
    }

    auto RenderPassEncoder::draw(uint32_t vertexCount, uint32_t startVertexLocation) -> void
    {
        applyState(m_viewports, m_scissors, m_lastBoundVao);

        rhi::DrawArguments args = {};
        args.vertexCount = vertexCount;
        args.startVertexLocation = startVertexLocation;
        args.instanceCount = 1;

        m_encoder->draw(args);
    }

    auto RenderPassEncoder::drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) -> void
    {
        applyState(m_viewports, m_scissors, m_lastBoundVao);

        rhi::DrawArguments args = {};
        args.vertexCount = vertexCount;
        args.instanceCount = instanceCount;
        args.startVertexLocation = startVertexLocation;
        args.startInstanceLocation = startInstanceLocation;
        m_encoder->draw(args);
    }

    auto RenderPassEncoder::drawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation) -> void
    {
        applyState(m_viewports, m_scissors, m_lastBoundVao);

        rhi::DrawArguments args = {};
        args.vertexCount = indexCount;
        args.instanceCount = 1;
        args.startIndexLocation = startIndexLocation;
        args.startVertexLocation = baseVertexLocation;
        m_encoder->drawIndexed(args);
    }

    auto RenderPassEncoder::drawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation) -> void
    {
        applyState(m_viewports, m_scissors, m_lastBoundVao);

        rhi::DrawArguments args = {};
        args.vertexCount = indexCount;
        args.instanceCount = instanceCount;
        args.startIndexLocation = startIndexLocation;
        args.startVertexLocation = baseVertexLocation;
        args.startInstanceLocation = startInstanceLocation;
        m_encoder->drawIndexed(args);
    }

    auto RenderPassEncoder::drawIndirect(uint32_t maxCommandCount, Buffer const* argBuffer, uint64_t argBufferOffset, Buffer const* countBuffer, uint64_t countBufferOffset) -> void
    {
        applyState(m_viewports, m_scissors, m_lastBoundVao);

        m_encoder->drawIndirect(
            maxCommandCount,
            rhi::BufferOffsetPair{argBuffer->getGfxBufferResource(), argBufferOffset},
            rhi::BufferOffsetPair{countBuffer ? countBuffer->getGfxBufferResource() : nullptr, countBufferOffset}
        );
    }

    auto RenderPassEncoder::drawIndexedIndirect(uint32_t maxCommandCount, Buffer const* argBuffer, uint64_t argBufferOffset, Buffer const* countBuffer, uint64_t countBufferOffset) -> void
    {
        applyState(m_viewports, m_scissors, m_lastBoundVao);

        m_encoder->drawIndexedIndirect(
            maxCommandCount,
            rhi::BufferOffsetPair{argBuffer->getGfxBufferResource(), argBufferOffset},
            rhi::BufferOffsetPair{countBuffer ? countBuffer->getGfxBufferResource() : nullptr, countBufferOffset}
        );
    }

    auto RenderPassEncoder::blit(core::ref<ShaderResourceView> const& src, core::ref<RenderTargetView> const& dst, uint4 srcRect, uint4 dstRect, TextureFilteringMode filter) -> void
    {
        // TODO
    }

    auto RenderPassEncoder::resolveResource(core::ref<Texture> const& src, core::ref<Texture> const& dst) -> void
    {
        // TODO
    }

    auto RenderPassEncoder::resolveSubresource(core::ref<Texture> const& src, uint32_t srcSubresource, core::ref<Texture> const& dst, uint32_t dstSubresource) -> void
    {
        // TODO
    }

    // ComputePassEncoder

    auto ComputePassEncoder::bindPipeline(ComputePipeline* pipeline, ProgramVariables* vars) -> void
    {
        m_lastBoundComputePipeline = pipeline;
        m_lastBoundComputeVars = vars;

        if (vars)
        {
            vars->prepareDescriptorSets(mp_context);
        }

        m_encoder->bindPipeline(pipeline->getGfxPipelineState(), vars ? vars->getShaderObject() : nullptr);
    }

    auto ComputePassEncoder::dispatch(uint3 const& dispatchSize) -> void
    {
        m_encoder->dispatchCompute(dispatchSize.x, dispatchSize.y, dispatchSize.z);
    }

    auto ComputePassEncoder::dispatchIndirect(Buffer const* argBuffer, uint64_t argBufferOffset) -> void
    {
        m_encoder->dispatchComputeIndirect(rhi::BufferOffsetPair{argBuffer->getGfxBufferResource(), argBufferOffset});
    }

    auto ComputePassEncoder::clearUAV(UnorderedAccessView const* uav, float4 const& value) -> void
    {
        // Typically done via resource commands, not inside compute pass in slang-rhi
        // TODO: Implement via command context or another mechanism
    }

    auto ComputePassEncoder::clearUAV(UnorderedAccessView const* uav, uint4 const& value) -> void
    {
        // TODO: Implement
    }

    auto ComputePassEncoder::clearUAVCounter(core::ref<Buffer> const& buffer, uint32_t value) -> void
    {
        // TODO: Implement
    }

    // RayTracingPassEncoder

    auto RayTracingPassEncoder::bindPipeline(RayTracingPipeline* pipeline, RtProgramVariables* vars) -> void
    {
        m_lastBoundPipeline = pipeline;
        m_lastBoundRtVars = vars;

        if (vars)
        {
            vars->prepareDescriptorSets(mp_context);
        }

        AP_ASSERT(vars, "RtProgramVariables must be provided when binding a ray tracing pipeline.");

        // In April Engine, ShaderTable management is separate. For now, we bind with nullptr table if not provided via vars.
        // Assuming RtProgramVariables will eventually provide access to its shader table.
        m_encoder->bindPipeline(pipeline->getGfxPipelineState(), vars->getShaderTable(), vars->getShaderObject());
    }

    auto RayTracingPassEncoder::raytrace(uint32_t width, uint32_t height, uint32_t depth) -> void
    {
        // rayGenShaderIndex is 0 for now as a default
        m_encoder->dispatchRays(0, width, height, depth);
    }

    auto RayTracingPassEncoder::buildAccelerationStructure(RtAccelerationStructure::BuildDesc const& desc, uint32_t postBuildInfoCount, RtAccelerationStructurePostBuildInfoDesc* postBuildInfoDescs) -> void
    {
        // TODO: Implement via IRayTracingPassEncoder or separate encoder if needed in slang-rhi
    }

    auto RayTracingPassEncoder::copyAccelerationStructure(RtAccelerationStructure* dest, RtAccelerationStructure* source, RtAccelerationStructureCopyMode mode) -> void
    {
        // TODO: Implement
    }


    // CommandContext::beginRenderPass

    auto CommandContext::beginRenderPass(
        ColorTargets const& pColorTargets,
        DepthStencilTarget const& pDepthStencilTarget
    ) -> core::ref<RenderPassEncoder>
    {
        auto queueType = mp_gfxCommandQueue->getType();
        AP_ASSERT(queueType == rhi::QueueType::Graphics, "Render passes can only be executed on a Graphics queue");

        auto pRenderPassEncoder = core::make_ref<RenderPassEncoder>(this, pColorTargets, pDepthStencilTarget);

        rhi::RenderPassDesc passDesc = {};
        passDesc.colorAttachments = pRenderPassEncoder->m_gfxColorAttachments.data();
        passDesc.colorAttachmentCount = (uint32_t)pRenderPassEncoder->m_gfxColorAttachments.size();
        if (pRenderPassEncoder->m_hasDepthStencil)
        {
            passDesc.depthStencilAttachment = &pRenderPassEncoder->m_gfxDepthStencilAttachment;
        }

        auto encoder = m_gfxEncoder->beginRenderPass(passDesc);

        if (!encoder) return nullptr;

        pRenderPassEncoder->setEncoder(encoder);

        m_commandsPending = true;

        return pRenderPassEncoder;
    }

    // CommandContext Pass Methods
    auto CommandContext::beginComputePass() -> core::ref<ComputePassEncoder>
    {
        auto queueType = mp_gfxCommandQueue->getType();

        AP_ASSERT(queueType == rhi::QueueType::Graphics,

            "Compute passes can only be executed on a Graphics queue (Compute queue not supported in RHI yet)");



        auto encoder = m_gfxEncoder->beginComputePass();

        if (!encoder) return nullptr;

        m_commandsPending = true;

        return core::make_ref<ComputePassEncoder>(this, encoder);
    }

    auto CommandContext::beginRayTracingPass() -> core::ref<RayTracingPassEncoder>
    {

        auto queueType = mp_gfxCommandQueue->getType();

        AP_ASSERT(queueType == rhi::QueueType::Graphics,
            "Ray tracing passes can only be executed on a Graphics queue (Compute queue not supported in RHI yet)");

        auto encoder = m_gfxEncoder->beginRayTracingPass();

        if (!encoder) return nullptr;

        m_commandsPending = true;

        return core::make_ref<RayTracingPassEncoder>(this, encoder);
    }

    auto CommandContext::clearRtv(RenderTargetView const* rtv, float4 const& color) -> void
    {
        float clearValue[4] = {color.x, color.y, color.z, color.w};
        auto encoder = m_gfxEncoder.get();
        // FIXME: make subresource range.
        encoder->clearTextureFloat(rtv->getGfxTexture(), rhi::kEntireTexture, clearValue);
        m_commandsPending = true;
    }

    auto CommandContext::clearDsv(DepthStencilView const* dsv, float depth, uint8_t stencil, bool clearDepth, bool clearStencil) -> void
    {
        m_gfxEncoder->clearTextureDepthStencil(dsv->getGfxTexture(), rhi::kEntireTexture, clearDepth, depth, clearStencil, stencil);
        m_commandsPending = true;
    }

    auto CommandContext::clearTexture(Texture* texture, float4 const& clearColor) -> void
    {
        AP_ASSERT(texture);
        // Simplified implementation based on CopyContext reference
        resourceBarrier(texture, Resource::State::RenderTarget);
        clearRtv(texture->getRTV().get(), clearColor);
    }

    auto CommandContext::clearBuffer(Buffer const* buffer) -> void
    {
        m_gfxEncoder->clearBuffer(buffer->getGfxBufferResource());
    }

    auto CommandContext::resourceBarrier(Resource const* resource, Resource::State newState, ResourceViewInfo const* viewInfo) -> bool
    {
        auto texture = dynamic_cast<Texture const*>(resource);
        if (texture)
        {
            bool globalBarrier = texture->isStateGlobal();
            if (viewInfo)
            {
                globalBarrier = globalBarrier && viewInfo->firstArraySlice == 0;
                globalBarrier = globalBarrier && viewInfo->mostDetailedMip == 0;
                globalBarrier = globalBarrier && viewInfo->mipCount == texture->getMipCount();
                globalBarrier = globalBarrier && viewInfo->arraySize == texture->getArraySize();
            }

            if (globalBarrier)
            {
                return textureBarrier(texture, newState);
            }
            else
            {
                return subresourceBarriers(texture, newState, viewInfo);
            }
        }
        else
        {
            auto buffer = dynamic_cast<Buffer const*>(resource);
            return bufferBarrier(buffer, newState);
        }
    }

    auto CommandContext::uavBarrier(Resource const* resource) -> void
    {
        auto resourceEncoder = m_gfxEncoder.get();

        if (resource->getType() == Resource::Type::Buffer)
        {
            rhi::IBuffer* gfxBuffer = static_cast<rhi::IBuffer*>(resource->getGfxResource());
            resourceEncoder->setBufferState(gfxBuffer, rhi::ResourceState::UnorderedAccess);
        }
        else
        {
            rhi::ITexture* gfxTexture = static_cast<rhi::ITexture*>(resource->getGfxResource());
            resourceEncoder->setTextureState(gfxTexture, rhi::ResourceState::UnorderedAccess);
        }
        m_commandsPending = true;
    }

    auto CommandContext::copyBuffer(Buffer const* dst, Buffer const* src) -> void
    {
        resourceBarrier(dst, Resource::State::CopyDest);
        resourceBarrier(src, Resource::State::CopySource);

        auto resourceEncoder = m_gfxEncoder.get();
        AP_ASSERT(src->getSize() <= dst->getSize());
        resourceEncoder->copyBuffer(dst->getGfxBufferResource(), 0, src->getGfxBufferResource(), 0, src->getSize());
        m_commandsPending = true;
    }

    auto CommandContext::copyTexture(Texture const* dst, Texture const* src) -> void
    {
        resourceBarrier(dst, Resource::State::CopyDest);
        resourceBarrier(src, Resource::State::CopySource);

        auto resourceEncoder = m_gfxEncoder.get();
        resourceEncoder->copyTexture(
            dst->getGfxTextureResource(), {},
            rhi::Offset3D{},
            src->getGfxTextureResource(),
            {},
            rhi::Offset3D{},
            rhi::Extent3D{}
        );
        m_commandsPending = true;
    }

    auto CommandContext::copySubresource(Texture const* dst, uint32_t dstSubresourceIdx, Texture const* src, uint32_t srcSubresourceIdx) -> void
    {
        copySubresourceRegion(dst, dstSubresourceIdx, src, srcSubresourceIdx, uint3(0), uint3(0), uint3(0xFFFFFFFF));
    }

    auto CommandContext::copyBufferRegion(Buffer const* dst, uint64_t dstOffset, Buffer const* src, uint64_t srcOffset, uint64_t numBytes) -> void
    {
        resourceBarrier(dst, Resource::State::CopyDest);
        resourceBarrier(src, Resource::State::CopySource);

        auto resourceEncoder = m_gfxEncoder.get();
        resourceEncoder->copyBuffer(dst->getGfxBufferResource(), dstOffset, src->getGfxBufferResource(), srcOffset, numBytes);
        m_commandsPending = true;
    }

    auto CommandContext::copySubresourceRegion(
        Texture const* dst,
        uint32_t dstSubresourceIdx,
        Texture const* src,
        uint32_t srcSubresourceIdx,
        uint3 const& dstOffset,
        uint3 const& srcOffset,
        uint3 const& size
    ) -> void
    {
        resourceBarrier(dst, Resource::State::CopyDest);
        resourceBarrier(src, Resource::State::CopySource);

        rhi::SubresourceRange dstSubresource = {};
        dstSubresource.layer = dst->getSubresourceArraySlice(dstSubresourceIdx);
        dstSubresource.layerCount = 1;
        dstSubresource.mip = dst->getSubresourceMipLevel(dstSubresourceIdx);
        dstSubresource.mipCount = 1;

        rhi::SubresourceRange srcSubresource = {};
        srcSubresource.layer = src->getSubresourceArraySlice(srcSubresourceIdx);
        srcSubresource.layerCount = 1;
        srcSubresource.mip = src->getSubresourceMipLevel(srcSubresourceIdx);
        srcSubresource.mipCount = 1;

        rhi::Extent3D copySize = {size.x, size.y, size.z};

        if (size.x == 0xFFFFFFFF)
        {
            copySize.width = (src->getWidth(srcSubresource.mip) - srcOffset.x);
            copySize.height = (src->getHeight(srcSubresource.mip) - srcOffset.y);
            copySize.depth = (src->getDepth(srcSubresource.mip) - srcOffset.z);
        }

        auto resourceEncoder = m_gfxEncoder.get();
        resourceEncoder->copyTexture(
            dst->getGfxTextureResource(),
            dstSubresource,
            rhi::Offset3D(dstOffset.x, dstOffset.y, dstOffset.z),
            src->getGfxTextureResource(),
            srcSubresource,
            rhi::Offset3D(srcOffset.x, srcOffset.y, srcOffset.z),
            copySize
        );
        m_commandsPending = true;
    }

    auto CommandContext::updateSubresourceData(
        Texture const* dst,
        uint32_t subresource,
        void const* pData,
        uint3 const& offset,
        uint3 const& size
    ) -> void
    {
        m_commandsPending = true;
        updateTextureSubresources(dst, subresource, 1, pData, offset, size);
    }

    auto CommandContext::updateTextureData(Texture const* texture, void const* data) -> void
    {
        m_commandsPending = true;
        uint32_t subresourceCount = texture->getArraySize() * texture->getMipCount();
        if (texture->getType() == Resource::Type::TextureCube)
        {
            subresourceCount *= 6;
        }
        updateTextureSubresources(texture, 0, subresourceCount, data);
    }

    auto CommandContext::updateBuffer(Buffer const* buffer, void const* data, size_t offset, size_t numBytes) -> void
    {
        if (numBytes == 0)
        {
            numBytes = buffer->getSize() - offset;
        }

        size_t adjustedNumBytes = numBytes;
        size_t adjustedOffset = offset;
        if (!buffer->adjustSizeOffsetParams(adjustedNumBytes, adjustedOffset))
        {
            AP_ERROR("CommandContext::updateBuffer() - size and offset are invalid. Nothing to update.");
            return;
        }

        bufferBarrier(buffer, Resource::State::CopyDest);
        auto resourceEncoder = m_gfxEncoder.get();
        resourceEncoder->uploadBufferData(buffer->getGfxBufferResource(), adjustedOffset, adjustedNumBytes, (void*)data);

        m_commandsPending = true;
    }

    auto CommandContext::readBuffer(Buffer const* buffer, void* data, size_t offset, size_t numBytes) -> void
    {
        if (numBytes == 0)
        {
            numBytes = buffer->getSize() - offset;
        }

        size_t adjustedNumBytes = numBytes;
        size_t adjustedOffset = offset;
        if (!buffer->adjustSizeOffsetParams(adjustedNumBytes, adjustedOffset))
        {
            AP_ERROR("CommandContext::readBuffer() - size and offset are invalid. Nothing to read.");
            return;
        }

        auto const& pReadBackHeap = mp_device->getReadBackHeap();
        auto allocation = pReadBackHeap->allocate(adjustedNumBytes);

        bufferBarrier(buffer, Resource::State::CopySource);

        auto resourceEncoder = m_gfxEncoder.get();
        resourceEncoder->copyBuffer(allocation.gfxBuffer, allocation.offset, buffer->getGfxBufferResource(), adjustedOffset, adjustedNumBytes);
        m_commandsPending = true;
        submit(true);

        std::memcpy(data, allocation.data, adjustedNumBytes);
        pReadBackHeap->release(allocation);
    }

    auto CommandContext::readTextureSubresource(Texture const* texture, uint32_t subresourceIndex) -> std::vector<uint8_t>
    {
        auto task = asyncReadTextureSubresource(texture, subresourceIndex);
        return task->getData();
    }

    auto CommandContext::asyncReadTextureSubresource(Texture const* texture, uint32_t subresourceIndex) -> ReadTextureTask::SharedPtr
    {
        return ReadTextureTask::create(this, texture, subresourceIndex);
    }

    auto CommandContext::pushDebugGroup(std::string_view name, float4 color) -> void
    {
        m_gfxEncoder->pushDebugGroup(name.data(), rhi::MarkerColor{color.r, color.g, color.b});
    }

    auto CommandContext::popDebugGroup() -> void
    {
        m_gfxEncoder->popDebugGroup();
    }

    auto CommandContext::insertDebugMarker(std::string_view name, float4 color) -> void
    {
        m_gfxEncoder->insertDebugMarker(name.data(), rhi::MarkerColor{color.r, color.g, color.b});
    }

    auto CommandContext::writeTimestamp(QueryHeap* pHeap, uint32_t index) -> void
    {
        m_gfxEncoder->writeTimestamp(pHeap->getGfxQueryPool(), index);
    }

    auto CommandContext::resolveQuery(QueryHeap* pHeap, uint32_t index, uint32_t count, Buffer const* buffer, uint64_t offset) -> void
    {
        m_gfxEncoder->resolveQuery(pHeap->getGfxQueryPool(), index, count, buffer->getGfxBufferResource(), offset);
    }

    auto CommandContext::textureBarrier(Texture const* texture, Resource::State newState) -> bool
    {
        auto resourceEncoder = m_gfxEncoder.get();
        bool recorded = false;
        if (texture->getGlobalState() != newState)
        {
            rhi::ITexture* gfxTexture = texture->getGfxTextureResource();
            resourceEncoder->setTextureState(gfxTexture, getGFXResourceState(newState));
            m_commandsPending = true;
            recorded = true;
        }
        texture->setGlobalState(newState);
        return recorded;
    }

    auto CommandContext::bufferBarrier(Buffer const* buffer, Resource::State newState) -> bool
    {
        AP_ASSERT(buffer);
        if (buffer->getMemoryType() != MemoryType::DeviceLocal)
        {
            return false;
        }
        bool recorded = false;
        if (buffer->getGlobalState() != newState)
        {
            auto resourceEncoder = m_gfxEncoder.get();
            rhi::IBuffer* gfxBuffer = buffer->getGfxBufferResource();
            resourceEncoder->setBufferState(gfxBuffer, getGFXResourceState(newState));
            buffer->setGlobalState(newState);
            m_commandsPending = true;
            recorded = true;
        }
        return recorded;
    }

    auto CommandContext::subresourceBarriers(Texture const* texture, Resource::State newState, ResourceViewInfo const* viewInfo) -> bool
    {
        ResourceViewInfo fullResource;
        bool setGlobal = false;
        if (viewInfo == nullptr)
        {
            fullResource.arraySize = texture->getArraySize();
            fullResource.firstArraySlice = 0;
            fullResource.mipCount = texture->getMipCount();
            fullResource.mostDetailedMip = 0;
            setGlobal = true;
            viewInfo = &fullResource;
        }

        bool entireViewTransitioned = true;

        for (uint32_t a = viewInfo->firstArraySlice; a < viewInfo->firstArraySlice + viewInfo->arraySize; a++)
        {
            for (uint32_t m = viewInfo->mostDetailedMip; m < viewInfo->mipCount + viewInfo->mostDetailedMip; m++)
            {
                Resource::State oldState = texture->getSubresourceState(a, m);
                if (oldState != newState)
                {
                    apiSubresourceBarrier(texture, newState, oldState, a, m);
                    texture->setSubresourceState(a, m, newState);
                    m_commandsPending = true;
                }
                else
                {
                    entireViewTransitioned = false;
                }
            }
        }
        if (setGlobal)
        {
            texture->setGlobalState(newState);
        }
        return entireViewTransitioned;
    }

    auto CommandContext::apiSubresourceBarrier(
        Texture const* texture,
        Resource::State newState,
        Resource::State oldState,
        uint32_t arraySlice,
        uint32_t mipLevel
    ) -> void
    {
        auto resourceEncoder = m_gfxEncoder.get();
        rhi::ITexture* gfxTexture = texture->getGfxTextureResource();
        rhi::SubresourceRange subresourceRange = {};
        subresourceRange.layer = arraySlice;
        subresourceRange.mip = mipLevel;
        subresourceRange.layerCount = 1;
        subresourceRange.mipCount = 1;
        resourceEncoder->setTextureState(gfxTexture, subresourceRange, getGFXResourceState(newState));
        m_commandsPending = true;
    }

    auto CommandContext::updateTextureSubresources(
        Texture const* texture,
        uint32_t firstSubresource,
        uint32_t subresourceCount,
        void const* pData,
        uint3 const& offset,
        uint3 const& size
    ) -> void
    {
        resourceBarrier(texture, Resource::State::CopyDest);

        bool copyRegion = glm::any(glm::notEqual(offset, uint3(0))) || glm::any(glm::notEqual(size, uint3(0xFFFFFFFF)));
        AP_ASSERT(subresourceCount == 1 || !copyRegion);
        uint8_t* dataPtr = (uint8_t*)pData;
        auto resourceEncoder = m_gfxEncoder.get();
        rhi::Offset3D gfxOffset = {offset.x, offset.y, offset.z};
        rhi::Extent3D gfxSize = {size.x, size.y, size.z};
        rhi::FormatInfo formatInfo = rhi::getFormatInfo(getGFXFormat(texture->getFormat()));

        for (uint32_t index = firstSubresource; index < firstSubresource + subresourceCount; index++)
        {
            rhi::SubresourceRange subresourceRange = {};
            subresourceRange.layer = texture->getSubresourceArraySlice(index);
            subresourceRange.mip = texture->getSubresourceMipLevel(index);
            subresourceRange.layerCount = 1;
            subresourceRange.mipCount = 1;

            if (!copyRegion)
            {
                gfxSize.width = (uint32_t)align_up(texture->getWidth(subresourceRange.mip), (size_t)formatInfo.blockWidth);
                gfxSize.height = (uint32_t)align_up(texture->getHeight(subresourceRange.mip), (size_t)formatInfo.blockHeight);
                gfxSize.depth = texture->getDepth(subresourceRange.mip);
            }

            rhi::SubresourceData data = {};
            data.data = dataPtr;
            data.rowPitch = static_cast<int64_t>(gfxSize.width) / formatInfo.blockWidth * formatInfo.blockSizeInBytes;
            data.slicePitch = data.rowPitch * (gfxSize.height / formatInfo.blockHeight);
            dataPtr += data.slicePitch * gfxSize.depth;
            resourceEncoder->uploadTextureData(texture->getGfxTextureResource(), subresourceRange, gfxOffset, gfxSize, &data, 1);
        }
    }

} // namespace april::graphics
