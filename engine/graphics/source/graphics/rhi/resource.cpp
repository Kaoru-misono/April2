// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "resource.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "render-device.hpp"
#include "rhi-tools.hpp"

#include <core/error/assert.hpp>
#include <core/log/logger.hpp>

namespace april::graphics
{
    Resource::Resource(core::ref<Device> const& p_device, Type type, ResourceBindFlags bindFlags, uint64_t size)
        : mp_device(p_device), m_type(type), m_bindFlags(bindFlags), m_size((size_t)size)
    {
        // Default initial state.
        if (enum_has_any_flags(m_bindFlags, ResourceBindFlags::RenderTarget))
        {
            m_state.global = State::RenderTarget;
        }
        else if (enum_has_any_flags(m_bindFlags, ResourceBindFlags::DepthStencil))
        {
            m_state.global = State::DepthStencil;
        }
        else if (enum_has_any_flags(m_bindFlags, ResourceBindFlags::UnorderedAccess))
        {
            m_state.global = State::UnorderedAccess;
        }
        else if (enum_has_any_flags(m_bindFlags, ResourceBindFlags::ShaderResource))
        {
            m_state.global = State::ShaderResource;
        }
    }

    auto to_string(Resource::Type type) -> std::string const
    {
        switch (type)
        {
        case Resource::Type::Buffer: return "Buffer";
        case Resource::Type::Texture1D: return "Texture1D";
        case Resource::Type::Texture2D: return "Texture2D";
        case Resource::Type::Texture3D: return "Texture3D";
        case Resource::Type::TextureCube: return "TextureCube";
        case Resource::Type::Texture2DMultisample: return "Texture2DMultisample";
        default: AP_UNREACHABLE(); return "";
        }
    }

    auto to_string(Resource::State state) -> std::string const
    {
        switch (state)
        {
        case Resource::State::Undefined: return "Undefined";
        case Resource::State::PreInitialized: return "PreInitialized";
        case Resource::State::Common: return "Common";
        case Resource::State::VertexBuffer: return "VertexBuffer";
        case Resource::State::ConstantBuffer: return "ConstantBuffer";
        case Resource::State::IndexBuffer: return "IndexBuffer";
        case Resource::State::RenderTarget: return "RenderTarget";
        case Resource::State::UnorderedAccess: return "UnorderedAccess";
        case Resource::State::DepthStencil: return "DepthStencil";
        case Resource::State::ShaderResource: return "ShaderResource";
        case Resource::State::StreamOut: return "StreamOut";
        case Resource::State::IndirectArg: return "IndirectArg";
        case Resource::State::CopyDest: return "CopyDest";
        case Resource::State::CopySource: return "CopySource";
        case Resource::State::ResolveDest: return "ResolveDest";
        case Resource::State::ResolveSource: return "ResolveSource";
        case Resource::State::Present: return "Present";
        case Resource::State::GenericRead: return "GenericRead";
        case Resource::State::Predication: return "Predication";
        case Resource::State::AccelerationStructureRead: return "AccelerationStructure";
        case Resource::State::AccelerationStructureWrite: return "AccelerationStructure";
        default: AP_UNREACHABLE();
        }
    }

    auto Resource::getDevice() const -> core::ref<Device>
    {
        return mp_device;
    }

    auto Resource::invalidateViews() const -> void
    {
        auto invalidateAll = [](auto& map)
        {
            for (auto const& item : map)
            {
                item.second->invalidate();
            }
            map.clear();
        };

        invalidateAll(m_srvs);
        invalidateAll(m_uavs);
        invalidateAll(m_rtvs);
        invalidateAll(m_dsvs);
    }

    auto Resource::setName(std::string const& name) -> void
    {
        m_name = name;
        // TODO:
        // mp_device->getGfxDevice()->->(m_name.c_str());
    }

    auto Resource::asTexture() -> core::ref<Texture>
    {
        AP_ASSERT(this);
        return core::ref<Texture>(dynamic_cast<Texture*>(this));
    }

    auto Resource::asBuffer() -> core::ref<Buffer>
    {
        AP_ASSERT(this);
        return core::ref<Buffer>(dynamic_cast<Buffer*>(this));
    }

    auto Resource::getGlobalState() const -> State
    {
        if (!m_state.isGlobal)
        {
            AP_WARN("Resource::getGlobalState() - the resource doesn't have a global state.");
            return State::Undefined;
        }
        return m_state.global;
    }

    auto Resource::getSubresourceState(uint32_t arraySlice, uint32_t mipLevel) const -> State
    {
        auto const* pTexture = dynamic_cast<Texture const*>(this);
        if (pTexture)
        {
            uint32_t subResource = pTexture->getSubresourceIndex(arraySlice, mipLevel);
            return (m_state.isGlobal) ? m_state.global : m_state.perSubresource[subResource];
        }
        else
        {
            AP_WARN("Calling Resource::getSubresourceState() on an object that is not a texture.");
            AP_ASSERT(m_state.isGlobal, "Buffers must always be in global state.");
            return m_state.global;
        }
    }

    auto Resource::setGlobalState(State newState) const -> void
    {
        m_state.isGlobal = true;
        m_state.global = newState;
    }

    auto Resource::setSubresourceState(uint32_t arraySlice, uint32_t mipLevel, State newState) const -> void
    {
        auto const* pTexture = dynamic_cast<Texture const*>(this);
        if (!pTexture)
        {
            AP_WARN("Calling Resource::setSubresourceState() on an object that is not a texture. This is invalid. Ignoring call");
            return;
        }

        if (m_state.isGlobal)
        {
            m_state.perSubresource.assign(pTexture->getSubresourceCount(), m_state.global);
        }
        m_state.isGlobal = false;
        m_state.perSubresource[pTexture->getSubresourceIndex(arraySlice, mipLevel)] = newState;
    }

    auto Resource::getSharedApiHandle() const -> SharedResourceApiHandle
    {
        gfx::InteropHandle handle = {};
        // TODO:
        // checkResult(getGfxResource()->getNativeHandle(&handle), "Failed to get shared handle");
        return (SharedResourceApiHandle)handle.handleValue;
    }

    auto Resource::getNativeHandle() const -> rhi::NativeHandle
    {
        rhi::NativeHandle gfxNativeHandle = {};
        checkResult(getGfxResource()->getNativeHandle(&gfxNativeHandle), "Failed to get native handle");

        return gfxNativeHandle;
    }

    auto Resource::breakStrongReferenceToDevice() -> void
    {
        mp_device.breakStrongReference();
    }
} // namespace april::graphics
