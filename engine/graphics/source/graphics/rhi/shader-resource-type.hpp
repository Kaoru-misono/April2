// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include <cstdint>
#include <core/tools/enum.hpp>

namespace april::graphics
{
    enum class ShaderResourceType : uint32_t
    {
        Unknown,
        TextureSrv,
        TextureUav,
        RawBufferSrv,
        RawBufferUav,
        TypedBufferSrv,
        TypedBufferUav,
        StructuredBufferSrv,
        StructuredBufferUav,
        ConstantBuffer,
        DepthStencilView,
        RenderTargetView,
        Sampler,
        AccelerationStructureSrv,
        Count
    };
    AP_ENUM_INFO(
        ShaderResourceType,
        {
            {ShaderResourceType::Unknown, "Unknown"},
            {ShaderResourceType::TextureSrv, "TextureSrv"},
            {ShaderResourceType::TextureUav, "TextureUav"},
            {ShaderResourceType::RawBufferSrv, "RawBufferSrv"},
            {ShaderResourceType::RawBufferUav, "RawBufferUav"},
            {ShaderResourceType::TypedBufferSrv, "TypedBufferSrv"},
            {ShaderResourceType::TypedBufferUav, "TypedBufferUav"},
            {ShaderResourceType::StructuredBufferSrv, "StructuredBufferSrv"},
            {ShaderResourceType::StructuredBufferUav, "StructuredBufferUav"},
            {ShaderResourceType::ConstantBuffer, "ConstantBuffer"},
            {ShaderResourceType::DepthStencilView, "DepthStencilView"},
            {ShaderResourceType::RenderTargetView, "RenderTargetView"},
            {ShaderResourceType::Sampler, "Sampler"},
            {ShaderResourceType::AccelerationStructureSrv, "AccelerationStructureSrv"},
        }
    );
    AP_ENUM_REGISTER(ShaderResourceType);
}
