#pragma once

#include <asset/asset-manager.hpp>
#include <core/foundation/object.hpp>
#include <core/math/type.hpp>
#include <graphics/program/program-variables.hpp>
#include <graphics/program/program.hpp>
#include <graphics/resources/static-mesh.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/graphics-pipeline.hpp>
#include <graphics/rhi/rasterizer-state.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/resource-views.hpp>
#include <graphics/rhi/texture.hpp>
#include <scene/scene-graph.hpp>

#include <unordered_map>

namespace april::scene
{
    class SceneRenderer final : public core::Object
    {
        APRIL_OBJECT(SceneRenderer)
    public:
        explicit SceneRenderer(core::ref<graphics::Device> device, asset::AssetManager* assetManager);

        auto setViewportSize(uint32_t width, uint32_t height) -> void;
        auto render(graphics::CommandContext* pContext, SceneGraph const& sceneGraph, float4 const& clearColor) -> void;

        auto getSceneColorTexture() const -> core::ref<graphics::Texture> { return m_sceneColor; }
        auto getSceneColorSrv() const -> core::ref<graphics::TextureView> { return m_sceneColorSrv; }

    private:
        auto ensureTarget(uint32_t width, uint32_t height) -> void;
        auto getMeshForPath(std::string const& path) -> core::ref<graphics::StaticMesh>;
        auto updateActiveCamera(SceneGraph const& sceneGraph) -> void;
        auto renderMeshEntities(core::ref<graphics::RenderPassEncoder> encoder, Registry const& registry) -> void;

        core::ref<graphics::Device> m_device{};
        asset::AssetManager* m_assetManager{};
        core::ref<graphics::Texture> m_sceneColor{};
        core::ref<graphics::Texture> m_sceneDepth{};
        core::ref<graphics::TextureView> m_sceneColorRtv{};
        core::ref<graphics::TextureView> m_sceneDepthDsv{};
        core::ref<graphics::TextureView> m_sceneColorSrv{};
        core::ref<graphics::GraphicsPipeline> m_pipeline{};
        core::ref<graphics::ProgramVariables> m_vars{};
        std::unordered_map<std::string, core::ref<graphics::StaticMesh>> m_meshCache{};
        float4x4 m_viewProjectionMatrix{1.0f};
        bool m_hasActiveCamera{false};
        uint32_t m_width{0};
        uint32_t m_height{0};
        graphics::ResourceFormat m_format{graphics::ResourceFormat::RGBA16Float};
    };
}
