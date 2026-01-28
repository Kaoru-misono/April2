// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "program/program-reflection.hpp"
#include "program/shader-variable.hpp"
#include "rhi/fwd.hpp"

#include <core/foundation/object.hpp>
#include <slang-gfx.h>
#include <slang-com-ptr.h>
#include <string>
#include <string_view>
#include <vector>
#include <map>

namespace april::graphics
{
    class CommandContext;
    class ProgramVersion;
    class Sampler;
    class RtAccelerationStructure;

    class ParameterBlock : public core::Object
    {
        APRIL_OBJECT(ParameterBlock)
    public:
        virtual ~ParameterBlock() override;

        using BindLocation = ParameterBlockReflection::BindLocation;

        static auto create(core::ref<Device> pDevice, core::ref<ProgramVersion const> const& pProgramVersion, core::ref<ReflectionType const> const& pType) -> core::ref<ParameterBlock>;
        static auto create(core::ref<Device> pDevice, core::ref<ParameterBlockReflection const> const& pReflection) -> core::ref<ParameterBlock>;
        static auto create(core::ref<Device> pDevice, core::ref<ProgramVersion const> const& pProgramVersion, std::string const& typeName) -> core::ref<ParameterBlock>;

        auto getShaderObject() const -> rhi::IShaderObject* { return m_shaderObject; }

        template <typename T>
        void setVariable(std::string_view name, T const& value)
        {
            getRootVariable()[name].set(value);
        }

        template <typename T>
        void setVariable(BindLocation const& bindLocation, T const& value);

        void setBlob(void const* pSrc, BindLocation const& bindLocation, size_t size);
        void setBlob(void const* pSrc, size_t offset, size_t size);

        // Buffer
        void setBuffer(std::string_view name, core::ref<Buffer> pBuffer);
        void setBuffer(BindLocation const& bindLocation, core::ref<Buffer> pBuffer);
        auto getBuffer(std::string_view name) const -> core::ref<Buffer>;
        auto getBuffer(BindLocation const& bindLocation) const -> core::ref<Buffer>;

        // Texture
        void setTexture(std::string_view name, core::ref<Texture> pTexture);
        void setTexture(BindLocation const& bindLocation, core::ref<Texture> pTexture);
        auto getTexture(std::string_view name) const -> core::ref<Texture>;
        auto getTexture(BindLocation const& bindLocation) const -> core::ref<Texture>;

        // SRV/UAV
        void setSrv(BindLocation const& bindLocation, core::ref<ShaderResourceView> pSrv);
        auto getSrv(BindLocation const& bindLocation) const -> core::ref<ShaderResourceView>;

        void setUav(BindLocation const& bindLocation, core::ref<UnorderedAccessView> pUav);
        auto getUav(BindLocation const& bindLocation) const -> core::ref<UnorderedAccessView>;

        // Acceleration Structure
        void setAccelerationStructure(BindLocation const& bindLocation, core::ref<RtAccelerationStructure> pAccl);
        auto getAccelerationStructure(BindLocation const& bindLocation) const -> core::ref<RtAccelerationStructure>;

        // Sampler
        void setSampler(std::string_view name, core::ref<Sampler> pSampler);
        void setSampler(BindLocation const& bindLocation, core::ref<Sampler> pSampler);
        auto getSampler(BindLocation const& bindLocation) const -> core::ref<Sampler>;
        auto getSampler(std::string_view name) const -> core::ref<Sampler>;

        // ParameterBlock
        void setParameterBlock(std::string_view name, core::ref<ParameterBlock> pBlock);
        void setParameterBlock(BindLocation const& bindLocation, core::ref<ParameterBlock> pBlock);
        auto getParameterBlock(std::string_view name) const -> core::ref<ParameterBlock>;
        auto getParameterBlock(BindLocation const& bindLocation) const -> core::ref<ParameterBlock>;

        auto getReflection() const -> core::ref<ParameterBlockReflection const> { return mp_reflector; }
        auto getElementType() const -> core::ref<ReflectionType const> { return mp_reflector->getElementType(); }
        auto getElementSize() const -> size_t;

        auto getVariableOffset(std::string_view varName) const -> TypedShaderVariableOffset;
        auto getRootVariable() const -> ShaderVariable;

        auto findMember(std::string_view varName) const -> ShaderVariable;
        auto findMember(uint32_t index) const -> ShaderVariable;

        auto getSize() const -> size_t;

        auto updateSpecialization() const -> bool;
        auto getSpecializedReflector() const -> core::ref<ParameterBlockReflection const> { return mp_specializedReflector; }

        auto prepareDescriptorSets(CommandContext* pCommandContext) -> bool;

        using SpecializationArgs = std::vector<slang::SpecializationArg>;
        void collectSpecializationArgs(SpecializationArgs& ioArgs) const;

        auto getRawData() const -> void const*;

    protected:
        ParameterBlock(core::ref<Device> pDevice, core::ref<ProgramVersion const> const& pProgramVersion, core::ref<ParameterBlockReflection const> const& pReflection);
        ParameterBlock(core::ref<Device> pDevice, core::ref<ProgramReflection const> const& pReflector);

        void initializeResourceBindings();
        void createConstantBuffers(ShaderVariable const& var);
        void checkForNestedTextureArrayResources();

        core::ref<Device> mp_device{};
        core::ref<ProgramVersion const> mp_programVersion{};
        core::ref<ParameterBlockReflection const> mp_reflector{};
        mutable core::ref<ParameterBlockReflection const> mp_specializedReflector{};

        Slang::ComPtr<rhi::IShaderObject> m_shaderObject{};

        std::map<rhi::ShaderOffset, core::ref<ParameterBlock>> m_parameterBlocks{};
        std::map<rhi::ShaderOffset, core::ref<ShaderResourceView>> m_srvs{};
        std::map<rhi::ShaderOffset, core::ref<UnorderedAccessView>> m_uavs{};
        std::map<rhi::ShaderOffset, core::ref<Resource>> m_resources{};
        std::map<rhi::ShaderOffset, core::ref<Sampler>> m_samplers{};
        std::map<rhi::ShaderOffset, core::ref<RtAccelerationStructure>> m_accelerationStructures{};
    };

    template <typename T>
    auto ShaderVariable::setImpl(T const& val) const -> void
    {
        m_parameterBlock->setVariable(m_offset, val);
    }
}
