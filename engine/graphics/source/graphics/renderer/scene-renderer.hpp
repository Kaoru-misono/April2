#pragma once

#include <core/foundation/object.hpp>
#include <core/math/type.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/graphics-pipeline.hpp>
#include <graphics/rhi/rasterizer-state.hpp>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/resource-views.hpp>
#include <graphics/rhi/texture.hpp>
#include <graphics/resources/static-mesh.hpp>
#include <graphics/program/program.hpp>
#include <graphics/program/program-variables.hpp>
#include <asset/asset-manager.hpp>

namespace april::graphics
{
    class SceneRenderer final : public core::Object
    {
        APRIL_OBJECT(SceneRenderer)
    public:
        explicit SceneRenderer(core::ref<Device> device, asset::AssetManager* assetManager);

        auto setViewportSize(uint32_t width, uint32_t height) -> void;
        auto render(CommandContext* pContext, float4 const& clearColor) -> void;

        auto getSceneColorTexture() const -> core::ref<Texture> { return m_sceneColor; }
        auto getSceneColorSrv() const -> core::ref<TextureView> { return m_sceneColorSrv; }

    private:
        auto ensureTarget(uint32_t width, uint32_t height) -> void;

        core::ref<Device> m_device{};
        asset::AssetManager* m_assetManager{};
        core::ref<Texture> m_sceneColor{};
        core::ref<Texture> m_sceneDepth{};
        core::ref<TextureView> m_sceneColorRtv{};
        core::ref<TextureView> m_sceneDepthDsv{};
        core::ref<TextureView> m_sceneColorSrv{};
        core::ref<GraphicsPipeline> m_pipeline{};
        core::ref<ProgramVariables> m_vars{};
        core::ref<StaticMesh> m_cubeMesh{};
        uint32_t m_width{0};
        uint32_t m_height{0};
        ResourceFormat m_format{ResourceFormat::RGBA16Float};
        float m_time{0.0f};
    };
}
