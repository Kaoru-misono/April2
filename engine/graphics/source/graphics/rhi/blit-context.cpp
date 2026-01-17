// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "blit-context.hpp"
#include "render-device.hpp"
#include "graphics/program/program.hpp"
#include "graphics/program/program-variables.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>

namespace april::graphics
{
    // Placeholder for FullScreenPass if it doesn't exist yet.
    // In a real port, this would be in its own header.
    class FullScreenPass : public core::Object
    {
    public:
        static auto create(core::ref<Device> device, ProgramDesc const& desc, DefineList const& defines) -> core::ref<FullScreenPass>
        {
            // TODO: Implement FullScreenPass
            return nullptr;
        }
        auto breakStrongReferenceToDevice() -> void {}
        auto getVars() const -> core::ref<ProgramVariables> { return nullptr; }
        auto getProgram() const -> core::ref<Program> { return nullptr; }
    };

    BlitContext::BlitContext(Device* device)
    {
        AP_ASSERT(device);

        // Init the blit data.
        DefineList defines = {
            {"SAMPLE_COUNT", "1"},
            {"COMPLEX_BLIT", "0"},
            {"SRC_INT", "0"},
            {"DST_INT", "0"},
        };
        ProgramDesc d;
        d.addShaderLibrary("engine/graphics/shader/core/blit-reduction.slang");
        d.vsEntryPoint("vsMain");
        d.psEntryPoint("psMain");
        pass = FullScreenPass::create(core::ref<Device>(device), d, defines);
        if (pass)
        {
            pass->breakStrongReferenceToDevice();
        }

        if (pass && pass->getVars())
        {
            blitParamsBuffer = pass->getVars()->getParameterBlock("BlitParamsCB");
            if (blitParamsBuffer)
            {
                offsetVariableOffset = blitParamsBuffer->getVariableOffset("gOffset");
                scaleVariableOffset = blitParamsBuffer->getVariableOffset("gScale");
            }
        }

        prevSrcRectOffset = float2(-1.0f);
        prevSrcRectScale = float2(-1.0f);

        Sampler::Desc desc;
        desc.setAddressingMode(TextureAddressingMode::Clamp, TextureAddressingMode::Clamp, TextureAddressingMode::Clamp);
        desc.setReductionMode(TextureReductionMode::Standard);
        desc.setFilterMode(TextureFilteringMode::Linear, TextureFilteringMode::Linear, TextureFilteringMode::Point);
        linearSampler = device->createSampler(desc);
        linearSampler->breakStrongReferenceToDevice();
        desc.setFilterMode(TextureFilteringMode::Point, TextureFilteringMode::Point, TextureFilteringMode::Point);
        pointSampler = device->createSampler(desc);
        pointSampler->breakStrongReferenceToDevice();

        // Min reductions.
        desc.setReductionMode(TextureReductionMode::Min);
        desc.setFilterMode(TextureFilteringMode::Linear, TextureFilteringMode::Linear, TextureFilteringMode::Point);
        linearMinSampler = device->createSampler(desc);
        linearMinSampler->breakStrongReferenceToDevice();
        desc.setFilterMode(TextureFilteringMode::Point, TextureFilteringMode::Point, TextureFilteringMode::Point);
        pointMinSampler = device->createSampler(desc);
        pointMinSampler->breakStrongReferenceToDevice();

        // Max reductions.
        desc.setReductionMode(TextureReductionMode::Max);
        desc.setFilterMode(TextureFilteringMode::Linear, TextureFilteringMode::Linear, TextureFilteringMode::Point);
        linearMaxSampler = device->createSampler(desc);
        linearMaxSampler->breakStrongReferenceToDevice();
        desc.setFilterMode(TextureFilteringMode::Point, TextureFilteringMode::Point, TextureFilteringMode::Point);
        pointMaxSampler = device->createSampler(desc);
        pointMaxSampler->breakStrongReferenceToDevice();

        if (pass && pass->getProgram())
        {
            auto const& pDefaultBlockReflection = pass->getProgram()->getReflector()->getDefaultParameterBlock();
            texBindLocation = pDefaultBlockReflection->getResourceBinding("gTex");
        }

        // Complex blit parameters
        if (blitParamsBuffer)
        {
            compTransVariableOffset[0] = blitParamsBuffer->getVariableOffset("gCompTransformR");
            compTransVariableOffset[1] = blitParamsBuffer->getVariableOffset("gCompTransformG");
            compTransVariableOffset[2] = blitParamsBuffer->getVariableOffset("gCompTransformB");
            compTransVariableOffset[3] = blitParamsBuffer->getVariableOffset("gCompTransformA");
            prevComponentsTransform[0] = float4(1.0f, 0.0f, 0.0f, 0.0f);
            prevComponentsTransform[1] = float4(0.0f, 1.0f, 0.0f, 0.0f);
            prevComponentsTransform[2] = float4(0.0f, 0.0f, 1.0f, 0.0f);
            prevComponentsTransform[3] = float4(0.0f, 0.0f, 0.0f, 1.0f);
            for (uint32_t i = 0; i < 4; i++)
            {
                blitParamsBuffer->setVariable(compTransVariableOffset[i], prevComponentsTransform[i]);
            }
        }
    }
} // namespace april::graphics
