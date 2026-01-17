#pragma once

#include "imgui-backend.hpp"
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/graphics-pipeline.hpp>
#include <graphics/program/program.hpp>
#include <graphics/program/program-variables.hpp>

struct ImDrawData;

namespace april::graphics
{
    class RenderPassEncoder;

    class SlangRHIImGuiBackend final : public IImGuiBackend
    {
    public:
        SlangRHIImGuiBackend() = default;
        ~SlangRHIImGuiBackend() override;

        auto init(core::ref<Device> pDevice, float dpiScale = 1.0f) -> void override;
        auto shutdown() -> void override;
        auto newFrame() -> void override;
        auto renderDrawData(ImDrawData* drawData, core::ref<RenderPassEncoder> pEncoder) -> void override;

    private:
        auto initResources() -> void;
        auto createFontsTexture() -> void;

    private:
        core::ref<Device> mp_device;

        core::ref<Program> m_program;
        core::ref<ProgramVariables> m_vars;
        core::ref<GraphicsPipeline> m_pipeline;
        core::ref<Texture> m_fontTexture;
        core::ref<Sampler> m_fontSampler;
        core::ref<VertexLayout> m_layout;

        struct FrameResources
        {
            core::ref<Buffer> vertexBuffer;
            core::ref<Buffer> indexBuffer;
            uint32_t vertexCount{0};
            uint32_t indexCount{0};
        };

        std::vector<FrameResources> m_frameResources;
        uint32_t m_frameIndex{0};
        float m_dpiScale{1.0f};
    };
}
