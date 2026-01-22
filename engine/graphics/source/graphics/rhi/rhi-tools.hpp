#pragma once

#include "fwd.hpp"
#include <core/log/logger.hpp>
#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-rhi.h>
#include <cstdlib>

namespace april
{
    template <typename T>
    inline auto checkValue(T value, char const* msg, ISlangBlob* diag = nullptr) -> void
    {
        if (!value)
        {
            AP_CRITICAL("{}", msg);
            if (diag) AP_CRITICAL("[Diagnostics]\n{}", static_cast<char const*>(diag->getBufferPointer()));
            std::exit(1);
        }
    }

    inline auto checkResult(SlangResult res, char const* msg, ISlangBlob* diag = nullptr) -> void
    {
        if (SLANG_FAILED(res))
        {
            AP_CRITICAL("{} (Error: {:#x})", msg, static_cast<uint32_t>(res));
            if (diag) AP_CRITICAL("[Diagnostics]\n{}", static_cast<char const*>(diag->getBufferPointer()));
            std::exit(1);
        }
    }

    inline auto diagnoseIfNeeded(ISlangBlob* diagnosticsBlob) -> void
    {
        if (diagnosticsBlob != nullptr)
        {
            AP_ERROR("{}", std::string((char const*)diagnosticsBlob->getBufferPointer()));
        }
    }
}

namespace april::graphics
{
    // Forward declarations
    enum class ResourceFormat : uint32_t;
    enum class MemoryType : uint32_t;
    enum class BufferUsage : uint32_t;
    enum class TextureUsage : uint32_t;

    auto getGFXFormat(ResourceFormat format) -> rhi::Format;
    
    // Resource::State is tricky to forward declare because it's nested.
    // We'll use a raw uint32_t or just include resource.hpp in the .cpp only.
    // But getGFXResourceState is used in many places.
}

#include "resource.hpp"

namespace april::graphics
{
    auto getGFXResourceState(Resource::State state) -> rhi::ResourceState;

    auto getGFXBufferUsage(BufferUsage usage) -> rhi::BufferUsage;
    auto getGFXTextureUsage(TextureUsage usage) -> rhi::TextureUsage;

    auto createBufferResource(
        core::ref<Device> p_device,
        Resource::State initState,
        size_t size,
        size_t elementSize,
        ResourceFormat format,
        BufferUsage usage,
        MemoryType memoryType
    ) -> Slang::ComPtr<rhi::IBuffer>;

    auto prepareGFXBufferDesc(
        rhi::BufferDesc& bufDesc,
        Resource::State initState,
        size_t size,
        size_t elementSize,
        ResourceFormat format,
        BufferUsage usage,
        MemoryType memoryType
    ) -> void;
}
