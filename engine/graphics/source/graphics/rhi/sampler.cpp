// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "sampler.hpp"
#include "render-device.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>

namespace april::graphics
{
    namespace
    {
        auto getGFXAddressMode(TextureAddressingMode mode) -> rhi::TextureAddressingMode
        {
            switch (mode)
            {
            case TextureAddressingMode::Border:
                return rhi::TextureAddressingMode::ClampToBorder;
            case TextureAddressingMode::Clamp:
                return rhi::TextureAddressingMode::ClampToEdge;
            case TextureAddressingMode::Mirror:
                return rhi::TextureAddressingMode::MirrorRepeat;
            case TextureAddressingMode::MirrorOnce:
                return rhi::TextureAddressingMode::MirrorOnce;
            case TextureAddressingMode::Wrap:
                return rhi::TextureAddressingMode::Wrap;
            default:
                AP_CRITICAL("Unreachable code reached!");
                return rhi::TextureAddressingMode::ClampToBorder;
            }
        }

        auto getGFXFilter(TextureFilteringMode filter) -> rhi::TextureFilteringMode
        {
            switch (filter)
            {
            case TextureFilteringMode::Linear:
                return rhi::TextureFilteringMode::Linear;
            case TextureFilteringMode::Point:
                return rhi::TextureFilteringMode::Point;
            default:
                AP_CRITICAL("Unreachable code reached!");
                return rhi::TextureFilteringMode::Point;
            }
        }

        auto getGFXReductionMode(TextureReductionMode mode) -> rhi::TextureReductionOp
        {
            switch (mode)
            {
            case TextureReductionMode::Standard:
                return rhi::TextureReductionOp::Average;
            case TextureReductionMode::Comparison:
                return rhi::TextureReductionOp::Comparison;
            case TextureReductionMode::Min:
                return rhi::TextureReductionOp::Minimum;
            case TextureReductionMode::Max:
                return rhi::TextureReductionOp::Maximum;
            default:
                return rhi::TextureReductionOp::Average;
            }
        }

        auto getGFXComparisonFunc(ComparisonFunc func) -> rhi::ComparisonFunc
        {
            switch (func)
            {
            case ComparisonFunc::Disabled: return rhi::ComparisonFunc::Never;
            case ComparisonFunc::Never: return rhi::ComparisonFunc::Never;
            case ComparisonFunc::Always: return rhi::ComparisonFunc::Always;
            case ComparisonFunc::Less: return rhi::ComparisonFunc::Less;
            case ComparisonFunc::Equal: return rhi::ComparisonFunc::Equal;
            case ComparisonFunc::NotEqual: return rhi::ComparisonFunc::NotEqual;
            case ComparisonFunc::LessEqual: return rhi::ComparisonFunc::LessEqual;
            case ComparisonFunc::Greater: return rhi::ComparisonFunc::Greater;
            case ComparisonFunc::GreaterEqual: return rhi::ComparisonFunc::GreaterEqual;
            default: AP_CRITICAL("Unreachable code reached!"); return rhi::ComparisonFunc::Never;
            }
        }
    } // namespace

    Sampler::Sampler(core::ref<Device> const& p_device, Desc const& desc) : mp_device(p_device), m_desc(desc)
    {
        rhi::SamplerDesc gfxDesc = {};
        gfxDesc.addressU = getGFXAddressMode(desc.addressModeU);
        gfxDesc.addressV = getGFXAddressMode(desc.addressModeV);
        gfxDesc.addressW = getGFXAddressMode(desc.addressModeW);

        static_assert(sizeof(gfxDesc.borderColor) == sizeof(desc.borderColor));
        std::memcpy(gfxDesc.borderColor, &desc.borderColor, sizeof(desc.borderColor));

        gfxDesc.comparisonFunc = getGFXComparisonFunc(desc.comparisonFunc);
        gfxDesc.magFilter = getGFXFilter(desc.magFilter);
        gfxDesc.maxAnisotropy = desc.maxAnisotropy;
        gfxDesc.maxLOD = desc.maxLod;
        gfxDesc.minFilter = getGFXFilter(desc.minFilter);
        gfxDesc.minLOD = desc.minLod;
        gfxDesc.mipFilter = getGFXFilter(desc.mipFilter);
        gfxDesc.mipLODBias = desc.lodBias;
        gfxDesc.reductionOp =
            (desc.comparisonFunc != ComparisonFunc::Disabled) ? rhi::TextureReductionOp::Comparison : getGFXReductionMode(desc.reductionMode);

        checkResult(mp_device->getGfxDevice()->createSampler(gfxDesc, m_gfxSampler.writeRef()), "Failed to create sampler state");
    }

    Sampler::~Sampler()
    {
    }

    auto Sampler::getNativeHandle() const -> rhi::NativeHandle
    {
        rhi::NativeHandle gfxNativeHandle = {};
        checkResult(m_gfxSampler->getNativeHandle(&gfxNativeHandle), "Failed to get native handle");

        return gfxNativeHandle;
    }

    auto Sampler::getApiMaxAnisotropy() -> uint32_t
    {
        return 16;
    }

    auto Sampler::breakStrongReferenceToDevice() -> void
    {
        mp_device.breakStrongReference();
    }
} // namespace april::graphics
