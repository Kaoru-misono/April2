#pragma once

#include <asset/asset-manager.hpp>
#include <core/foundation/object.hpp>
#include <core/math/type.hpp>
#include <graphics/material/i-material.hpp>
#include <graphics/program/program-variables.hpp>
#include <graphics/program/program.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/graphics-pipeline.hpp>
#include <graphics/rhi/rasterizer-state.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/resource-views.hpp>
#include <graphics/rhi/sampler.hpp>
#include <graphics/rhi/texture.hpp>
#include <scene/renderer/frame-snapshot-buffer.hpp>
#include <scene/renderer/render-resource-registry.hpp>
#include <scene/renderer/render-types.hpp>
#include <unordered_set>
#include <cstdint>


namespace april::scene
{
    class SceneRenderer final : public core::Object
    {
        APRIL_OBJECT(SceneRenderer)
    public:
        explicit SceneRenderer(core::ref<graphics::Device> device, asset::AssetManager* assetManager);

        auto setViewportSize(uint32_t width, uint32_t height) -> void;
        auto render(graphics::CommandContext* pContext, FrameSnapshot const& snapshot, float4 const& clearColor) -> void;

        auto getSceneColorTexture() const -> core::ref<graphics::Texture>;
        auto getSceneColorSrv() const -> core::ref<graphics::TextureView>;
        auto getResourceRegistry() -> RenderResourceRegistry&;
        auto acquireSnapshotForWrite() -> FrameSnapshot&;
        auto submitSnapshot() -> void;
        auto getSnapshotForRead() const -> FrameSnapshot const&;

    private:
        auto ensureTarget(uint32_t width, uint32_t height) -> void;
        auto createDefaultResources() -> void;
        auto renderMeshInstances(core::ref<graphics::RenderPassEncoder> encoder, FrameSnapshot const& snapshot) -> void;

        core::ref<graphics::Device> m_device{};
        core::ref<graphics::Texture> m_sceneColor{};
        core::ref<graphics::Texture> m_sceneDepth{};
        core::ref<graphics::TextureView> m_sceneColorRtv{};
        core::ref<graphics::TextureView> m_sceneDepthDsv{};
        core::ref<graphics::TextureView> m_sceneColorSrv{};
        core::ref<graphics::GraphicsPipeline> m_pipeline{};
        core::ref<graphics::Program> m_program{};
        core::ref<graphics::ProgramVariables> m_vars{};
        RenderResourceRegistry m_resources{};
        FrameSnapshotBuffer m_snapshotBuffer{};
        float4x4 m_viewProjectionMatrix{1.0f};
        float3 m_cameraPosition{0.0f, 0.0f, 0.0f};
        uint32_t m_width{0};
        uint32_t m_height{0};
        graphics::ResourceFormat m_format{graphics::ResourceFormat::RGBA16Float};

        // Default/fallback resources
        core::ref<graphics::Texture> m_defaultWhiteTexture{};
        core::ref<graphics::Sampler> m_defaultSampler{};
        core::ref<graphics::IMaterial> m_defaultMaterial{};

        // Track missing material slots to avoid log spam.
        std::unordered_set<uint64_t> m_missingMaterialSlots{};
        std::unordered_set<uint64_t> m_invalidMaterialIndices{};
    };
}
