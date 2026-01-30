// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "fwd.hpp"
#include "handles.hpp"
#include "resource-views.hpp"

#include <core/foundation/object.hpp>
#include <core/tools/enum.hpp>

#include <string>
#include <unordered_map>
#include <vector>
#include <slang-rhi.h>

namespace april::graphics
{
    class Texture;
    class Buffer;
    class ParameterBlock;
    struct ResourceViewInfo;

    class Resource : public core::Object
    {
        APRIL_OBJECT(Resource)
    public:
        /**
         * Resource types. Notice there are no array types. Array are controlled using the array size parameter on texture creation.
         */
        enum class Type
        {
            Buffer,               ///< Buffer. Can be bound to all shader-stages
            Texture1D,
            Texture1DArray,
            Texture2D,
            Texture2DArray,
            Texture2DMS,
            Texture2DMSArray,
            Texture3D,
            TextureCube,
            TextureCubeArray,
        };

        /**
         * Resource state. Keeps track of how the resource was last used
         */
        enum class State : uint32_t
        {
            Undefined,
            PreInitialized,
            Common,
            VertexBuffer,
            ConstantBuffer,
            IndexBuffer,
            RenderTarget,
            UnorderedAccess,
            DepthStencil,
            ShaderResource,
            StreamOut,
            IndirectArg,
            CopyDest,
            CopySource,
            ResolveDest,
            ResolveSource,
            Present,
            GenericRead,
            Predication,
            AccelerationStructureRead,
            AccelerationStructureWrite,
        };

        /**
         * Default value used in create*() methods
         */
        static constexpr uint32_t kMaxPossible = RenderTargetView::kMaxPossible;

        virtual ~Resource() = default;

        auto getDevice() const -> core::ref<Device>;

        auto isStateGlobal() const -> bool { return m_state.isGlobal; }

        /**
         * Get the current state. This is only valid if isStateGlobal() returns true
         */
        auto getGlobalState() const -> State;

        /**
         * Get a subresource state
         */
        auto getSubresourceState(uint32_t arraySlice, uint32_t mipLevel) const -> State;

        /**
         * Get the resource type
         */
        auto getType() const -> Type { return m_type; }

        /**
         * Get the resource
         */
        virtual auto getGfxResource() const -> rhi::IResource* = 0;

        /**
         * Returns the native API handle:
         * - D3D12: ID3D12Resource*
         * - Vulkan: VkBuffer or VkImage
         */
        auto getNativeHandle() const -> rhi::NativeHandle;

        /**
         * Get a shared resource API handle.
         *
         * The handle will be created on-demand if it does not already exist.
         * Throws if a shared handle cannot be created for this resource.
         */
        auto getSharedApiHandle() const -> SharedResourceApiHandle;

        struct ViewInfoHashFunc
        {
            auto operator()(ResourceViewInfo const& v) const -> std::size_t
            {
                return (size_t) (
                    ((std::hash<uint32_t>()(v.firstArraySlice) ^ (std::hash<uint32_t>()(v.arraySize) << 1)) >> 1) ^
                    (std::hash<uint32_t>()(v.mipCount) << 1) ^ (std::hash<uint32_t>()(v.mostDetailedMip) << 3) ^
                    (std::hash<uint64_t>()(v.offset) << 5) ^ (std::hash<uint64_t>()(v.size) << 7)
                );
            }
        };

        /**
         * Get the size of the resource
         */
        auto getSize() const -> size_t { return m_size; }

        /**
         * Invalidate and release all of the resource views
         */
        auto invalidateViews() const -> void;

        /**
         * Set the resource name
         */
        auto setName(std::string const& name) -> void;

        /**
         * Get the resource name
         */
        auto getName() const -> std::string const& { return m_name; }

        /**
         * Get a SRV/UAV for the entire resource.
         * Buffer and Texture have overloads which allow you to create a view into part of the resource
         */
        virtual auto getSRV() -> core::ref<ShaderResourceView> = 0;
        virtual auto getUAV() -> core::ref<UnorderedAccessView> = 0;

        /**
         * Conversions to derived classes
         */
        auto asTexture() -> core::ref<Texture>;
        auto asBuffer() -> core::ref<Buffer>;

        auto breakStrongReferenceToDevice() -> void;

    protected:
        friend class CommandContext;

        Resource(core::ref<Device> const& p_device, Type type, uint64_t size);

        core::BreakableReference<Device> mp_device;
        Type m_type;

        struct ResourceState
        {
            bool isGlobal{true};
            State global{State::Undefined};
            std::vector<State> perSubresource;
        };
        mutable ResourceState m_state;

        auto setSubresourceState(uint32_t arraySlice, uint32_t mipLevel, State newState) const -> void;
        auto setGlobalState(State newState) const -> void;

        size_t m_size{0}; ///< Size of the resource in bytes.
        std::string m_name{};
        mutable SharedResourceApiHandle m_sharedApiHandle{0};

        mutable std::unordered_map<ResourceViewInfo, core::ref<ShaderResourceView>, ViewInfoHashFunc> m_srvs;
        mutable std::unordered_map<ResourceViewInfo, core::ref<RenderTargetView>, ViewInfoHashFunc> m_rtvs;
        mutable std::unordered_map<ResourceViewInfo, core::ref<DepthStencilView>, ViewInfoHashFunc> m_dsvs;
        mutable std::unordered_map<ResourceViewInfo, core::ref<UnorderedAccessView>, ViewInfoHashFunc> m_uavs;
    };

    auto to_string(Resource::Type type) -> std::string const;
    auto to_string(Resource::State state) -> std::string const;

    AP_ENUM_CLASS_OPERATORS(Resource::State);
}
