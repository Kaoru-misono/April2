// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "program-reflection.hpp"
#include "rhi/fwd.hpp"
#include "rhi/ray-tracing-acceleration-structure.hpp"

#include <string_view>
#include <cstddef>

namespace april::graphics
{
    class ParameterBlock;
    class Texture;
    class Buffer;
    class Sampler;
    class ResourceView;
    class RtAccelerationStructure;

    // Renamed from ShaderVar
    class ShaderVariable
    {
    public:
        ShaderVariable() = default;
        ShaderVariable(ShaderVariable const& other) = default;
        explicit ShaderVariable(ParameterBlock* pObject, TypedShaderVariableOffset const& offset);
        explicit ShaderVariable(ParameterBlock* pObject);

        auto isValid() const -> bool { return m_offset.isValid(); }
        auto getType() const -> ReflectionType const* { return m_offset.getType(); }
        auto getOffset() const -> TypedShaderVariableOffset { return m_offset; }
        auto getByteOffset() const -> size_t { return m_offset.getUniform().getByteOffset(); }

        auto operator[](std::string_view name) const -> ShaderVariable;
        auto operator[](size_t index) const -> ShaderVariable;

        auto findMember(std::string_view name) const -> ShaderVariable;
        auto hasMember(std::string_view name) const -> bool { return findMember(name).isValid(); }

        auto findMember(uint32_t index) const -> ShaderVariable;
        auto hasMember(uint32_t index) const -> bool { return findMember(index).isValid(); }

        template <typename T>
        auto set(T const& val) const -> void { setImpl<T>(val); }

        template <typename T>
        auto operator=(T const& val) const -> void { setImpl(val); }

        auto setBlob(void const* data, size_t size) const -> void;

        template <typename T>
        auto setBlob(T const& val) const -> void { setBlob(&val, sizeof(val)); }

        // Resource binding
        auto setBuffer(core::ref<Buffer> pBuffer) const -> void;
        auto getBuffer() const -> core::ref<Buffer>;
        operator core::ref<Buffer>() const { return getBuffer(); }

        auto setTexture(core::ref<Texture> pTexture) const -> void;
        auto getTexture() const -> core::ref<Texture>;
        operator core::ref<Texture>() const { return getTexture(); }

        auto setSrv(core::ref<ResourceView> pSrv) const -> void;
        auto getSrv() const -> core::ref<ResourceView>;

        auto setUav(core::ref<ResourceView> pUav) const -> void;
        auto getUav() const -> core::ref<ResourceView>;

        auto setAccelerationStructure(core::ref<RtAccelerationStructure> pAccl) const -> void;
        auto getAccelerationStructure() const -> core::ref<RtAccelerationStructure>;

        auto setSampler(core::ref<Sampler> pSampler) const -> void;
        auto getSampler() const -> core::ref<Sampler>;
        operator core::ref<Sampler>() const; // Requires implementation

        auto setParameterBlock(core::ref<ParameterBlock> pBlock) const -> void;
        auto getParameterBlock() const -> core::ref<ParameterBlock>;

        operator TypedShaderVariableOffset() const { return m_offset; }
        operator UniformShaderVariableOffset() const { return m_offset.getUniform(); }

        auto operator[](TypedShaderVariableOffset const& offset) const -> ShaderVariable;
        auto operator[](UniformShaderVariableOffset const& offset) const -> ShaderVariable;

        auto getRawData() const -> void const*;

    private:
        ParameterBlock* m_parameterBlock{nullptr};
        TypedShaderVariableOffset m_offset{};

        // Implementation helpers...
        template <typename T>
        auto setImpl(T const& val) const -> void;
        auto setImpl(core::ref<Texture> const& pTexture) const -> void;
        auto setImpl(core::ref<Sampler> const& pSampler) const -> void;
        auto setImpl(core::ref<Buffer> const& pBuffer) const -> void;
        auto setImpl(core::ref<ParameterBlock> const& pBlock) const -> void;
    };
}
