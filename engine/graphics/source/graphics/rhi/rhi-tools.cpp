// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "rhi-tools.hpp"
#include "format.hpp"
#include "resource.hpp"
#include "render-device.hpp"

#include <core/error/assert.hpp>
#include <core/tools/enum-flags.hpp>

namespace april::graphics
{
    auto getGFXFormat(ResourceFormat format) -> rhi::Format
    {
        switch (format)
        {
        case ResourceFormat::BC1Unorm: return rhi::Format::BC1Unorm;
        case ResourceFormat::BC1UnormSrgb: return rhi::Format::BC1UnormSrgb;
        case ResourceFormat::BC2Unorm: return rhi::Format::BC2Unorm;
        case ResourceFormat::BC2UnormSrgb: return rhi::Format::BC2UnormSrgb;
        case ResourceFormat::BC3Unorm: return rhi::Format::BC3Unorm;
        case ResourceFormat::BC3UnormSrgb: return rhi::Format::BC3UnormSrgb;
        case ResourceFormat::BC4Snorm: return rhi::Format::BC4Snorm;
        case ResourceFormat::BC4Unorm: return rhi::Format::BC4Unorm;
        case ResourceFormat::BC5Snorm: return rhi::Format::BC5Snorm;
        case ResourceFormat::BC5Unorm: return rhi::Format::BC5Unorm;
        case ResourceFormat::BC6HS16: return rhi::Format::BC6HSfloat;
        case ResourceFormat::BC6HU16: return rhi::Format::BC6HUfloat;
        case ResourceFormat::BC7Unorm: return rhi::Format::BC7Unorm;
        case ResourceFormat::BC7UnormSrgb: return rhi::Format::BC7UnormSrgb;
        case ResourceFormat::BGRA4Unorm: return rhi::Format::BGRA4Unorm;
        case ResourceFormat::BGRA8Unorm: return rhi::Format::BGRA8Unorm;
        case ResourceFormat::BGRA8UnormSrgb: return rhi::Format::BGRA8UnormSrgb;
        case ResourceFormat::BGRX8Unorm: return rhi::Format::BGRX8Unorm;
        case ResourceFormat::BGRX8UnormSrgb: return rhi::Format::BGRX8UnormSrgb;
        case ResourceFormat::D16Unorm: return rhi::Format::D16Unorm;
        case ResourceFormat::D32Float: return rhi::Format::D32Float;
        case ResourceFormat::D32FloatS8Uint: return rhi::Format::D32FloatS8Uint;
        case ResourceFormat::R11G11B10Float: return rhi::Format::R11G11B10Float;
        case ResourceFormat::R16Float: return rhi::Format::R16Float;
        case ResourceFormat::R16Int: return rhi::Format::R16Sint;
        case ResourceFormat::R16Snorm: return rhi::Format::R16Snorm;
        case ResourceFormat::R16Uint: return rhi::Format::R16Uint;
        case ResourceFormat::R16Unorm: return rhi::Format::R16Unorm;
        case ResourceFormat::R32Float: return rhi::Format::R32Float;
        case ResourceFormat::R32Int: return rhi::Format::R32Sint;
        case ResourceFormat::R32Uint: return rhi::Format::R32Uint;
        case ResourceFormat::R5G6B5Unorm: return rhi::Format::B5G6R5Unorm;
        case ResourceFormat::R8Int: return rhi::Format::R8Sint;
        case ResourceFormat::R8Snorm: return rhi::Format::R8Snorm;
        case ResourceFormat::R8Uint: return rhi::Format::R8Uint;
        case ResourceFormat::R8Unorm: return rhi::Format::R8Unorm;
        case ResourceFormat::RG16Float: return rhi::Format::RG16Float;
        case ResourceFormat::RG16Int: return rhi::Format::RG16Sint;
        case ResourceFormat::RG16Snorm: return rhi::Format::RG16Snorm;
        case ResourceFormat::RG16Uint: return rhi::Format::RG16Uint;
        case ResourceFormat::RG16Unorm: return rhi::Format::RG16Unorm;
        case ResourceFormat::RG32Float: return rhi::Format::RG32Float;
        case ResourceFormat::RG32Int: return rhi::Format::RG32Sint;
        case ResourceFormat::RG32Uint: return rhi::Format::RG32Uint;
        case ResourceFormat::RG8Int: return rhi::Format::RG8Sint;
        case ResourceFormat::RG8Snorm: return rhi::Format::RG8Snorm;
        case ResourceFormat::RG8Uint: return rhi::Format::RG8Uint;
        case ResourceFormat::RG8Unorm: return rhi::Format::RG8Unorm;
        case ResourceFormat::RGB10A2Uint: return rhi::Format::RGB10A2Uint;
        case ResourceFormat::RGB10A2Unorm: return rhi::Format::RGB10A2Unorm;
        case ResourceFormat::RGB32Float: return rhi::Format::RGB32Float;
        case ResourceFormat::RGB32Int: return rhi::Format::RGB32Sint;
        case ResourceFormat::RGB32Uint: return rhi::Format::RGB32Uint;
        case ResourceFormat::RGB5A1Unorm: return rhi::Format::Undefined;
        case ResourceFormat::RGB9E5Float: return rhi::Format::RGB9E5Ufloat;
        case ResourceFormat::RGBA16Float: return rhi::Format::RGBA16Float;
        case ResourceFormat::RGBA16Int: return rhi::Format::RGBA16Sint;
        case ResourceFormat::RGBA16Uint: return rhi::Format::RGBA16Uint;
        case ResourceFormat::RGBA16Unorm: return rhi::Format::RGBA16Unorm;
        case ResourceFormat::RGBA16Snorm: return rhi::Format::RGBA16Snorm;
        case ResourceFormat::RGBA32Float: return rhi::Format::RGBA32Float;
        case ResourceFormat::RGBA32Int: return rhi::Format::RGBA32Sint;
        case ResourceFormat::RGBA32Uint: return rhi::Format::RGBA32Uint;
        case ResourceFormat::RGBA8Int: return rhi::Format::RGBA8Sint;
        case ResourceFormat::RGBA8Snorm: return rhi::Format::RGBA8Snorm;
        case ResourceFormat::RGBA8Uint: return rhi::Format::RGBA8Uint;
        case ResourceFormat::RGBA8Unorm: return rhi::Format::RGBA8Unorm;
        case ResourceFormat::RGBA8UnormSrgb: return rhi::Format::RGBA8UnormSrgb;
        default: return rhi::Format::Undefined;
        }
    }

    auto getGFXResourceState(Resource::State state) -> rhi::ResourceState
    {
        switch (state)
        {
        case Resource::State::Undefined: return rhi::ResourceState::Undefined;
        case Resource::State::Common: return rhi::ResourceState::General;
        case Resource::State::VertexBuffer: return rhi::ResourceState::VertexBuffer;
        case Resource::State::ConstantBuffer: return rhi::ResourceState::ConstantBuffer;
        case Resource::State::IndexBuffer: return rhi::ResourceState::IndexBuffer;
        case Resource::State::RenderTarget: return rhi::ResourceState::RenderTarget;
        case Resource::State::UnorderedAccess: return rhi::ResourceState::UnorderedAccess;
        case Resource::State::DepthStencil: return rhi::ResourceState::DepthWrite;
        case Resource::State::ShaderResource: return rhi::ResourceState::ShaderResource;
        case Resource::State::StreamOut: return rhi::ResourceState::StreamOutput;
        case Resource::State::IndirectArg: return rhi::ResourceState::IndirectArgument;
        case Resource::State::CopyDest: return rhi::ResourceState::CopyDestination;
        case Resource::State::CopySource: return rhi::ResourceState::CopySource;
        case Resource::State::ResolveDest: return rhi::ResourceState::ResolveDestination;
        case Resource::State::ResolveSource: return rhi::ResourceState::ResolveSource;
        case Resource::State::Present: return rhi::ResourceState::Present;
        case Resource::State::GenericRead: return rhi::ResourceState::General;
        case Resource::State::Predication: return rhi::ResourceState::General;;
        case Resource::State::AccelerationStructureRead: return rhi::ResourceState::AccelerationStructureRead;
        case Resource::State::AccelerationStructureWrite: return rhi::ResourceState::AccelerationStructureWrite;
        default: AP_UNREACHABLE();
        }
    }

    auto getGFXBufferUsage(BufferUsage usage) -> rhi::BufferUsage
    {
        rhi::BufferUsage res = rhi::BufferUsage::None;
        if (enum_has_any_flags(usage, BufferUsage::VertexBuffer)) res |= rhi::BufferUsage::VertexBuffer;
        if (enum_has_any_flags(usage, BufferUsage::IndexBuffer)) res |= rhi::BufferUsage::IndexBuffer;
        if (enum_has_any_flags(usage, BufferUsage::ConstantBuffer)) res |= rhi::BufferUsage::ConstantBuffer;
        if (enum_has_any_flags(usage, BufferUsage::ShaderResource)) res |= rhi::BufferUsage::ShaderResource;
        if (enum_has_any_flags(usage, BufferUsage::UnorderedAccess)) res |= rhi::BufferUsage::UnorderedAccess;
        if (enum_has_any_flags(usage, BufferUsage::IndirectArgument)) res |= rhi::BufferUsage::IndirectArgument;
        if (enum_has_any_flags(usage, BufferUsage::AccelerationStructure)) res |= rhi::BufferUsage::AccelerationStructure;
        return res;
    }

    auto getGFXTextureUsage(TextureUsage usage) -> rhi::TextureUsage
    {
        rhi::TextureUsage res = rhi::TextureUsage::None;
        if (enum_has_any_flags(usage, TextureUsage::ShaderResource)) res |= rhi::TextureUsage::ShaderResource;
        if (enum_has_any_flags(usage, TextureUsage::UnorderedAccess)) res |= rhi::TextureUsage::UnorderedAccess;
        if (enum_has_any_flags(usage, TextureUsage::RenderTarget)) res |= rhi::TextureUsage::RenderTarget;
        if (enum_has_any_flags(usage, TextureUsage::DepthStencil)) res |= rhi::TextureUsage::DepthStencil;
        if (enum_has_any_flags(usage, TextureUsage::Present)) res |= rhi::TextureUsage::Present;
        if (enum_has_any_flags(usage, TextureUsage::Shared)) res |= rhi::TextureUsage::Shared;
        res |= rhi::TextureUsage::CopySource | rhi::TextureUsage::CopyDestination;
        return res;
    }

    auto prepareGFXBufferDesc(
        rhi::BufferDesc& bufDesc,
        Resource::State initState,
        size_t size,
        size_t elementSize,
        ResourceFormat format,
        BufferUsage usage,
        MemoryType memoryType
    ) -> void
    {
        bufDesc.size = size;
        bufDesc.elementSize = static_cast<uint32_t>(elementSize);
        bufDesc.format = getGFXFormat(format);
        switch (memoryType)
        {
        case MemoryType::DeviceLocal:
            bufDesc.memoryType = rhi::MemoryType::DeviceLocal;
            break;
        case MemoryType::ReadBack:
            bufDesc.memoryType = rhi::MemoryType::ReadBack;
            break;
        case MemoryType::Upload:
            bufDesc.memoryType = rhi::MemoryType::Upload;
            break;
        default:
            AP_UNREACHABLE();
            break;
        }
        bufDesc.usage = getGFXBufferUsage(usage);
        bufDesc.defaultState = getGFXResourceState(initState);
    }

    auto createBufferResource(
        core::ref<Device> p_device,
        Resource::State initState,
        size_t size,
        size_t elementSize,
        ResourceFormat format,
        BufferUsage usage,
        MemoryType memoryType
    ) -> Slang::ComPtr<rhi::IBuffer>
    {
        AP_ASSERT(p_device);

        // Create the buffer
        rhi::BufferDesc bufDesc = {};
        prepareGFXBufferDesc(bufDesc, initState, size, elementSize, format, usage, memoryType);

        Slang::ComPtr<rhi::IBuffer> p_apiHandle;
        checkResult(p_device->getGfxDevice()->createBuffer(bufDesc, nullptr, p_apiHandle.writeRef()), "Failed to create buffer resource");
        AP_ASSERT(p_apiHandle);

        return p_apiHandle;
    }
} // namespace april::graphics
