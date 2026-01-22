// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "parameter-block.hpp"
#include "sampler.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "ray-tracing-acceleration-structure.hpp"
#include "render-device.hpp"
#include "command-context.hpp"
#include "rhi-tools.hpp"
#include "program/program-version.hpp"
#include "program/shader-variable.hpp"

#include <core/error/assert.hpp>
#include <core/tools/enum-flags.hpp>

namespace april::graphics
{
    namespace
    {
        auto getGFXShaderOffset(UniformShaderVariableOffset const& offset) -> rhi::ShaderOffset
        {
            rhi::ShaderOffset result;
            result.bindingArrayIndex = 0;
            result.bindingRangeIndex = 0;
            result.uniformOffset = offset.getByteOffset();
            return result;
        }

        auto getGFXShaderOffset(ParameterBlock::BindLocation const& bindLoc) -> rhi::ShaderOffset
        {
            rhi::ShaderOffset gfxOffset = {};
            gfxOffset.bindingArrayIndex = bindLoc.getResourceArrayIndex();
            gfxOffset.bindingRangeIndex = bindLoc.getResourceRangeIndex();
            gfxOffset.uniformOffset = bindLoc.getUniform().getByteOffset();
            return gfxOffset;
        }

        auto isSrvType(ReflectionType const* pType) -> bool
        {
            AP_ASSERT(pType);
            auto resourceType = pType->unwrapArray()->asResourceType();
            if (!resourceType || resourceType->getType() == ReflectionResourceType::Type::Sampler ||
                resourceType->getType() == ReflectionResourceType::Type::ConstantBuffer)
            {
                return false;
            }

            switch (resourceType->getShaderAccess())
            {
            case ReflectionResourceType::ShaderAccess::Read:
                return true;
            case ReflectionResourceType::ShaderAccess::ReadWrite:
                return false;
            default:
                AP_UNREACHABLE();
            }
        }

        auto isUavType(ReflectionType const* pType) -> bool
        {
            AP_ASSERT(pType);
            auto resourceType = pType->unwrapArray()->asResourceType();
            if (!resourceType || resourceType->getType() == ReflectionResourceType::Type::Sampler ||
                resourceType->getType() == ReflectionResourceType::Type::ConstantBuffer)
            {
                return false;
            }

            switch (resourceType->getShaderAccess())
            {
            case ReflectionResourceType::ShaderAccess::Read:
                return false;
            case ReflectionResourceType::ShaderAccess::ReadWrite:
                return true;
            default:
                AP_UNREACHABLE();
            }
        }

        auto isSamplerType(ReflectionType const* pType) -> bool
        {
            AP_ASSERT(pType);
            auto resourceType = pType->unwrapArray()->asResourceType();
            return (resourceType && resourceType->getType() == ReflectionResourceType::Type::Sampler);
        }

        auto isAccelerationStructureType(ReflectionType const* pType) -> bool
        {
            AP_ASSERT(pType);
            auto resourceType = pType->unwrapArray()->asResourceType();
            return (resourceType && resourceType->getType() == ReflectionResourceType::Type::AccelerationStructure);
        }

        auto isParameterBlockType(ReflectionType const* pType) -> bool
        {
            AP_ASSERT(pType);
            auto resourceType = pType->unwrapArray()->asResourceType();
            return (resourceType && resourceType->getType() == ReflectionResourceType::Type::ConstantBuffer);
        }

        auto isConstantBufferType(ReflectionType const* pType) -> bool
        {
            AP_ASSERT(pType);
            auto resourceType = pType->unwrapArray()->asResourceType();
            return (resourceType && resourceType->getType() == ReflectionResourceType::Type::ConstantBuffer);
        }

        auto prepareResource(CommandContext* pContext, Resource* pResource, bool isUav) -> void
        {
            if (!pResource) return;

            auto pBuffer = pResource->asBuffer();
            if (isUav && pBuffer && pBuffer->getUAVCounter())
            {
                pContext->resourceBarrier(pBuffer->getUAVCounter().get(), Resource::State::UnorderedAccess);
                pContext->uavBarrier(pBuffer->getUAVCounter().get());
            }

            bool isAccelerationStructure = pBuffer && enum_has_any_flags(pBuffer->getUsage(), BufferUsage::AccelerationStructure);
            bool insertBarrier = !isAccelerationStructure;
            if (insertBarrier)
            {
                insertBarrier = !pContext->resourceBarrier(pResource, isUav ? Resource::State::UnorderedAccess : Resource::State::ShaderResource);
            }

            if (insertBarrier && isUav)
            {
                pContext->uavBarrier(pResource);
            }
        }
    } // namespace

    auto ParameterBlock::create(
        core::ref<Device> pDevice,
        core::ref<ProgramVersion const> const& pProgramVersion,
        core::ref<ReflectionType const> const& pType
    ) -> core::ref<ParameterBlock>
    {
        AP_ASSERT(pType, "Can't create a parameter block without type information");
        auto pReflection = ParameterBlockReflection::create(pProgramVersion.get(), pType);
        return create(pDevice, pReflection);
    }

    auto ParameterBlock::create(core::ref<Device> pDevice, core::ref<ParameterBlockReflection const> const& pReflection) -> core::ref<ParameterBlock>
    {
        AP_ASSERT(pReflection);
        // TODO(@skallweit) we convert the weak pointer to a shared pointer here because we tie
        // the lifetime of the parameter block to the lifetime of the program version.
        // The ownership for programs/versions/kernels and parameter blocks needs to be revisited.
        return core::ref<ParameterBlock>(new ParameterBlock(pDevice, core::ref<ProgramVersion const>(pReflection->getProgramVersion()), pReflection));
    }

    auto ParameterBlock::create(
        core::ref<Device> pDevice,
        core::ref<ProgramVersion const> const& pProgramVersion,
        std::string const& typeName
    ) -> core::ref<ParameterBlock>
    {
        AP_ASSERT(pProgramVersion);
        return create(pDevice, pProgramVersion, pProgramVersion->getReflector()->findType(typeName));
    }

    ParameterBlock::ParameterBlock(core::ref<Device> pDevice, core::ref<ProgramReflection const> const& pReflector)
        : mp_device(pDevice), mp_programVersion(pReflector->getProgramVersion()), mp_reflector(pReflector->getDefaultParameterBlock())
    {
        checkResult(mp_device->getGfxDevice()->createRootShaderObject(
            pReflector->getProgramVersion()->getKernels(mp_device.get(), nullptr)->getGfxShaderProgram(), m_shaderObject.writeRef()
        ), "Failed to create mutable root shader object");
        initializeResourceBindings();
        createConstantBuffers(getRootVariable());
    }

    ParameterBlock::ParameterBlock(
        core::ref<Device> pDevice,
        core::ref<ProgramVersion const> const& pProgramVersion,
        core::ref<ParameterBlockReflection const> const& pReflection
    )
        : mp_device(pDevice), mp_programVersion(pProgramVersion), mp_reflector(pReflection)
    {
        checkResult(mp_device->getGfxDevice()->createShaderObjectFromTypeLayout(
            pReflection->getElementType()->getSlangTypeLayout(), m_shaderObject.writeRef()
        ), "Failed to create mutable shader object from type layout");
        initializeResourceBindings();
        createConstantBuffers(getRootVariable());
    }

    ParameterBlock::~ParameterBlock() = default;

    auto ParameterBlock::getRootVariable() const -> ShaderVariable
    {
        return ShaderVariable(const_cast<ParameterBlock*>(this));
    }

    auto ParameterBlock::findMember(std::string_view varName) const -> ShaderVariable
    {
        return getRootVariable().findMember(varName);
    }

    auto ParameterBlock::findMember(uint32_t index) const -> ShaderVariable
    {
        return getRootVariable().findMember(index);
    }

    auto ParameterBlock::getElementSize() const -> size_t
    {
        return mp_reflector->getElementType()->getByteSize();
    }

    auto ParameterBlock::getVariableOffset(std::string_view varName) const -> TypedShaderVariableOffset
    {
        return getElementType()->getZeroOffset()[varName];
    }

    auto ParameterBlock::createConstantBuffers(ShaderVariable const& var) -> void
    {
        auto pType = var.getType();
        if (pType->getResourceRangeCount() == 0) return;

        switch (pType->getKind())
        {
        case ReflectionType::Kind::Struct:
        {
            auto pStructType = pType->asStructType();
            uint32_t memberCount = pStructType->getMemberCount();
            for (uint32_t i = 0; i < memberCount; ++i)
            {
                createConstantBuffers(var[i]);
            }
        }
        break;
        case ReflectionType::Kind::Resource:
        {
            auto pResourceType = pType->asResourceType();
            switch (pResourceType->getType())
            {
            case ReflectionResourceType::Type::ConstantBuffer:
            {
                auto pCB = ParameterBlock::create(mp_device, pResourceType->getParameterBlockReflector());
                var.setParameterBlock(pCB);
            }
            break;
            default:
                break;
            }
        }
        break;
        default:
            break;
        }
    }

    auto ParameterBlock::initializeResourceBindings() -> void
    {
        if (mp_device->getType() == Device::Type::Vulkan)
        {
            checkForNestedTextureArrayResources();
        }

        for (uint32_t i = 0; i < mp_reflector->getResourceRangeCount(); i++)
        {
            auto info = mp_reflector->getResourceRangeBindingInfo(i);
            auto range = mp_reflector->getResourceRange(i);
            for (uint32_t arrayIndex = 0; arrayIndex < range.count; arrayIndex++)
            {
                rhi::ShaderOffset offset = {};
                offset.bindingRangeIndex = i;
                offset.bindingArrayIndex = arrayIndex;
                switch (range.descriptorType)
                {
                case ShaderResourceType::Sampler:
                    m_shaderObject->setBinding(offset, mp_device->getDefaultSampler()->getGfxSamplerState());
                    break;
                case ShaderResourceType::TextureSrv:
                case ShaderResourceType::TextureUav:
                case ShaderResourceType::RawBufferSrv:
                case ShaderResourceType::RawBufferUav:
                case ShaderResourceType::TypedBufferSrv:
                case ShaderResourceType::TypedBufferUav:
                case ShaderResourceType::StructuredBufferUav:
                case ShaderResourceType::StructuredBufferSrv:
                case ShaderResourceType::AccelerationStructureSrv:
                    m_shaderObject->setBinding(offset, {});
                    break;
                default: break;
                }
            }
        }
    }

    auto ParameterBlock::checkForNestedTextureArrayResources() -> void
    {
        auto reflectorStruct = mp_reflector->getElementType()->asStructType();
        if (reflectorStruct)
        {
            for (uint32_t i = 0; i < reflectorStruct->getMemberCount(); i++)
            {
                auto const& member = reflectorStruct->getMember(i);

                auto elementType = member->getType();
                int depth = 0;
                while (elementType->getKind() == ReflectionType::Kind::Array)
                {
                    elementType = elementType->asArrayType()->getElementType().get();
                    depth++;
                }

                if (depth > 1)
                {
                    auto resourceType = elementType->asResourceType();
                    if (resourceType && resourceType->getType() == ReflectionResourceType::Type::Texture)
                    {
                        AP_ERROR("Nested texture array '{}' detected in parameter block. This will fail silently on Vulkan.", member->getName());
                    }
                }
            }
        }
    }

    auto ParameterBlock::setBlob(void const* pSrc, BindLocation const& bindLocation, size_t size) -> void
    {
        if (!isConstantBufferType(bindLocation.getType()))
        {
            rhi::ShaderOffset gfxOffset = getGFXShaderOffset(bindLocation);
            checkResult(m_shaderObject->setData(gfxOffset, pSrc, size), "Failed to set data to shader object");
        }
        else
        {
            AP_ERROR("Error trying to set a blob to a non constant buffer variable.");
        }
    }

    auto ParameterBlock::setBlob(void const* pSrc, size_t offset, size_t size) -> void
    {
        rhi::ShaderOffset gfxOffset = {};
        gfxOffset.uniformOffset = (uint32_t)offset;
        checkResult(m_shaderObject->setData(gfxOffset, pSrc, size), "Failed to set data to shader object");
    }

    template<typename T>
    void setVariableInternal(
        ParameterBlock* pBlock,
        const ParameterBlock::BindLocation& bindLocation,
        const T& value,
        ReflectionBasicType::Type type,
        ReflectionBasicType::Type implicitType = ReflectionBasicType::Type::Unknown
    )
    {
        const ReflectionBasicType* basicType = bindLocation.getType()->unwrapArray()->asBasicType();
        if (!basicType)
            AP_CRITICAL("Error trying to set a variable that is not a basic type.");
        ReflectionBasicType::Type expectedType = basicType->getType();
        // Check types. Allow implicit conversions from signed to unsigned types.
        if (type != expectedType && implicitType != expectedType)
            AP_CRITICAL(
                "Error trying to set a variable with a different type than the one in the program (expected {}, got {}).",
                enumToString(expectedType),
                enumToString(type)
            );
        size_t size = sizeof(T);
        size_t expectedSize = basicType->getByteSize();
        if (size != expectedSize)
            AP_CRITICAL(
                "Error trying to set a variable with a different size than the one in the program (expected {} bytes, got {}).",
                expectedSize,
                size
            );
        auto gfxOffset = getGFXShaderOffset(bindLocation);
        checkResult(pBlock->getShaderObject()->setData(gfxOffset, &value, size), "Parameter block set data failed");
    }

    template<typename T>
    auto ParameterBlock::setVariable(const BindLocation& bindLocation, const T& value) -> void
    {
        (void) bindLocation;
        (void) value;
        AP_CRITICAL("Not implement yet.");
    }

    #define DEFINE_SET_VARIABLE(ctype, basicType, implicitType)                                           \
        template<>                                                                                        \
        auto ParameterBlock::setVariable(const BindLocation& bindLocation, const ctype& value) -> void    \
        {                                                                                                 \
            setVariableInternal<ctype>(this, bindLocation, value, basicType, implicitType);               \
        }

    DEFINE_SET_VARIABLE(uint32_t, ReflectionBasicType::Type::Uint, ReflectionBasicType::Type::Int);
    DEFINE_SET_VARIABLE(uint2, ReflectionBasicType::Type::Uint2, ReflectionBasicType::Type::Int2);
    DEFINE_SET_VARIABLE(uint3, ReflectionBasicType::Type::Uint3, ReflectionBasicType::Type::Int3);
    DEFINE_SET_VARIABLE(uint4, ReflectionBasicType::Type::Uint4, ReflectionBasicType::Type::Int4);

    DEFINE_SET_VARIABLE(int32_t, ReflectionBasicType::Type::Int, ReflectionBasicType::Type::Uint);
    DEFINE_SET_VARIABLE(int2, ReflectionBasicType::Type::Int2, ReflectionBasicType::Type::Uint2);
    DEFINE_SET_VARIABLE(int3, ReflectionBasicType::Type::Int3, ReflectionBasicType::Type::Uint3);
    DEFINE_SET_VARIABLE(int4, ReflectionBasicType::Type::Int4, ReflectionBasicType::Type::Uint4);

    DEFINE_SET_VARIABLE(float, ReflectionBasicType::Type::Float, ReflectionBasicType::Type::Unknown);
    DEFINE_SET_VARIABLE(float2, ReflectionBasicType::Type::Float2, ReflectionBasicType::Type::Unknown);
    DEFINE_SET_VARIABLE(float3, ReflectionBasicType::Type::Float3, ReflectionBasicType::Type::Unknown);
    DEFINE_SET_VARIABLE(float4, ReflectionBasicType::Type::Float4, ReflectionBasicType::Type::Unknown);

    // DEFINE_SET_VARIABLE(float1x4, ReflectionBasicType::Type::Float1x4, ReflectionBasicType::Type::Unknown);
    // DEFINE_SET_VARIABLE(float2x4, ReflectionBasicType::Type::Float2x4, ReflectionBasicType::Type::Unknown);
    // DEFINE_SET_VARIABLE(float3x4, ReflectionBasicType::Type::Float3x4, ReflectionBasicType::Type::Unknown);
    DEFINE_SET_VARIABLE(float4x4, ReflectionBasicType::Type::Float4x4, ReflectionBasicType::Type::Unknown);

    DEFINE_SET_VARIABLE(uint64_t, ReflectionBasicType::Type::Uint64, ReflectionBasicType::Type::Int64);

    #undef DEFINE_SET_VARIABLE

    // Template specialization to allow setting booleans on a parameter block.
    // On the host side a bool is 1B and the device 4B. We cast bools to 32-bit integers here.
    // Note that this applies to our boolN vectors as well, which are currently 1B per element.

    template<>
    auto ParameterBlock::setVariable(const BindLocation& bindLocation, const bool& value) -> void
    {
        uint v = value ? 1 : 0;
        setVariableInternal(this, bindLocation, v, ReflectionBasicType::Type::Bool);
    }

    template<>
    auto ParameterBlock::setVariable(const BindLocation& bindLocation, const bool2& value) -> void
    {
        uint2 v = {value.x ? 1 : 0, value.y ? 1 : 0};
        setVariableInternal(this, bindLocation, v, ReflectionBasicType::Type::Bool2);
    }

    template<>
    auto ParameterBlock::setVariable(const BindLocation& bindLocation, const bool3& value) -> void
    {
        uint3 v = {value.x ? 1 : 0, value.y ? 1 : 0, value.z ? 1 : 0};
        setVariableInternal(this, bindLocation, v, ReflectionBasicType::Type::Bool3);
    }

    template<>
    auto ParameterBlock::setVariable(const BindLocation& bindLocation, const bool4& value) -> void
    {
        uint4 v = {value.x ? 1 : 0, value.y ? 1 : 0, value.z ? 1 : 0, value.w ? 1 : 0};
        setVariableInternal(this, bindLocation, v, ReflectionBasicType::Type::Bool4);
    }

    auto ParameterBlock::setBuffer(std::string_view name, core::ref<Buffer> pBuffer) -> void
    {
        getRootVariable()[name].setBuffer(pBuffer);
    }

    auto ParameterBlock::setBuffer(BindLocation const& bindLoc, core::ref<Buffer> pBuffer) -> void
    {
        rhi::ShaderOffset gfxOffset = getGFXShaderOffset(bindLoc);
        if (isUavType(bindLoc.getType()))
        {
            if (pBuffer && !enum_has_any_flags(pBuffer->getUsage(), BufferUsage::UnorderedAccess))
            {
                AP_ERROR("Trying to bind buffer created without UnorderedAccess flag as a UAV.");
            }
            auto pUAV = pBuffer ? pBuffer->getUAV() : nullptr;
            m_shaderObject->setBinding(gfxOffset, pUAV ? pUAV->getGfxBinding() : rhi::Binding{});
            m_uavs[gfxOffset] = pUAV;
        }
        else if (isSrvType(bindLoc.getType()))
        {
            if (pBuffer && !enum_has_any_flags(pBuffer->getUsage(), BufferUsage::ShaderResource))
            {
                AP_ERROR("Trying to bind buffer created without ShaderResource flag as an SRV.");
            }
            auto pSRV = pBuffer ? pBuffer->getSRV() : nullptr;
            m_shaderObject->setBinding(gfxOffset, pSRV ? pSRV->getGfxBinding() : rhi::Binding{});
            m_srvs[gfxOffset] = pSRV;
        }
        else
        {
            AP_ERROR("Error trying to bind buffer to a non SRV/UAV variable.");
        }

        m_resources[gfxOffset] = pBuffer;
    }

    auto ParameterBlock::getBuffer(std::string_view name) const -> core::ref<Buffer>
    {
        return getRootVariable()[name].getBuffer();
    }

    auto ParameterBlock::getBuffer(BindLocation const& bindLoc) const -> core::ref<Buffer>
    {
        rhi::ShaderOffset gfxOffset = getGFXShaderOffset(bindLoc);
        if (isUavType(bindLoc.getType()))
        {
            auto iter = m_uavs.find(gfxOffset);
            if (iter == m_uavs.end()) return nullptr;
            auto pResource = iter->second->getResource();
            return pResource ? pResource->asBuffer() : nullptr;
        }
        else if (isSrvType(bindLoc.getType()))
        {
            auto iter = m_srvs.find(gfxOffset);
            if (iter == m_srvs.end()) return nullptr;
            auto pResource = iter->second->getResource();
            return pResource ? pResource->asBuffer() : nullptr;
        }
        else
        {
            AP_ERROR("Error trying to get buffer from a non SRV/UAV variable.");
            return nullptr;
        }
    }

    auto ParameterBlock::setTexture(std::string_view name, core::ref<Texture> pTexture) -> void
    {
        getRootVariable()[name].setTexture(pTexture);
    }

    auto ParameterBlock::setTexture(BindLocation const& bindLocation, core::ref<Texture> pTexture) -> void
    {
        rhi::ShaderOffset gfxOffset = getGFXShaderOffset(bindLocation);
        if (isUavType(bindLocation.getType()))
        {
            if (pTexture && !enum_has_any_flags(pTexture->getUsage(), TextureUsage::UnorderedAccess))
            {
                AP_ERROR("Trying to bind texture created without UnorderedAccess flag as a UAV.");
            }
            auto pUAV = pTexture ? pTexture->getUAV() : nullptr;
            m_shaderObject->setBinding(gfxOffset, pUAV ? pUAV->getGfxBinding() : rhi::Binding{});
            m_uavs[gfxOffset] = pUAV;
        }
        else if (isSrvType(bindLocation.getType()))
        {
            if (pTexture && !enum_has_any_flags(pTexture->getUsage(), TextureUsage::ShaderResource))
            {
                AP_ERROR("Trying to bind texture created without ShaderResource flag as an SRV.");
            }
            auto pSRV = pTexture ? pTexture->getSRV() : nullptr;
            m_shaderObject->setBinding(gfxOffset, pSRV ? pSRV->getGfxBinding() : rhi::Binding{});
            m_srvs[gfxOffset] = pSRV;
        }
        else
        {
            AP_ERROR("Error trying to bind texture to a non SRV/UAV variable.");
        }

        m_resources[gfxOffset] = pTexture;
    }

    auto ParameterBlock::getTexture(std::string_view name) const -> core::ref<Texture>
    {
        return getRootVariable()[name].getTexture();
    }

    auto ParameterBlock::getTexture(BindLocation const& bindLocation) const -> core::ref<Texture>
    {
        rhi::ShaderOffset gfxOffset = getGFXShaderOffset(bindLocation);
        if (isUavType(bindLocation.getType()))
        {
            auto iter = m_uavs.find(gfxOffset);
            if (iter == m_uavs.end()) return nullptr;
            auto pResource = iter->second->getResource();
            return pResource ? pResource->asTexture() : nullptr;
        }
        else if (isSrvType(bindLocation.getType()))
        {
            auto iter = m_srvs.find(gfxOffset);
            if (iter == m_srvs.end()) return nullptr;
            auto pResource = iter->second->getResource();
            return pResource ? pResource->asTexture() : nullptr;
        }
        else
        {
            AP_ERROR("Error trying to get texture from a non SRV/UAV variable.");
            return nullptr;
        }
    }

    auto ParameterBlock::setSrv(BindLocation const& bindLocation, core::ref<ShaderResourceView> pSrv) -> void
    {
        if (isSrvType(bindLocation.getType()))
        {
            rhi::ShaderOffset gfxOffset = getGFXShaderOffset(bindLocation);
            m_shaderObject->setBinding(gfxOffset, pSrv->getGfxBinding());
            m_srvs[gfxOffset] = pSrv;
            m_resources[gfxOffset] = core::ref<Resource>(pSrv ? pSrv->getResource() : nullptr);
        }
        else
        {
            AP_ERROR("Error trying to bind an SRV to a non SRV variable.");
        }
    }

    auto ParameterBlock::getSrv(BindLocation const& bindLocation) const -> core::ref<ShaderResourceView>
    {
        if (isSrvType(bindLocation.getType()))
        {
            rhi::ShaderOffset gfxOffset = getGFXShaderOffset(bindLocation);
            auto iter = m_srvs.find(gfxOffset);
            if (iter == m_srvs.end()) return nullptr;
            return iter->second;
        }
        else
        {
            AP_ERROR("Error trying to get an SRV from a non SRV variable.");
            return nullptr;
        }
    }

    auto ParameterBlock::setUav(BindLocation const& bindLocation, core::ref<UnorderedAccessView> pUav) -> void
    {
        if (isUavType(bindLocation.getType()))
        {
            rhi::ShaderOffset gfxOffset = getGFXShaderOffset(bindLocation);
            m_shaderObject->setBinding(gfxOffset, pUav ? pUav->getGfxBinding() : rhi::Binding{});
            m_uavs[gfxOffset] = pUav;
            m_resources[gfxOffset] = core::ref<Resource>(pUav ? pUav->getResource() : nullptr);
        }
        else
        {
            AP_ERROR("Error trying to bind a UAV to a non UAV variable.");
        }
    }

    auto ParameterBlock::getUav(BindLocation const& bindLocation) const -> core::ref<UnorderedAccessView>
    {
        if (isUavType(bindLocation.getType()))
        {
            rhi::ShaderOffset gfxOffset = getGFXShaderOffset(bindLocation);
            auto iter = m_uavs.find(gfxOffset);
            if (iter == m_uavs.end()) return nullptr;
            return iter->second;
        }
        else
        {
            AP_ERROR("Error trying to get a UAV from a non UAV variable.");
            return nullptr;
        }
    }

    auto ParameterBlock::setAccelerationStructure(BindLocation const& bindLocation, core::ref<RtAccelerationStructure> pAccl) -> void
    {
        if (isAccelerationStructureType(bindLocation.getType()))
        {
            rhi::ShaderOffset gfxOffset = getGFXShaderOffset(bindLocation);
            m_accelerationStructures[gfxOffset] = pAccl;
            m_shaderObject->setBinding(gfxOffset, pAccl ? pAccl->getGfxAccelerationStructure() : nullptr);
        }
        else
        {
            AP_ERROR("Error trying to bind an acceleration structure to a non acceleration structure variable.");
        }
    }

    auto ParameterBlock::getAccelerationStructure(BindLocation const& bindLocation) const -> core::ref<RtAccelerationStructure>
    {
        if (isAccelerationStructureType(bindLocation.getType()))
        {
            rhi::ShaderOffset gfxOffset = getGFXShaderOffset(bindLocation);
            auto iter = m_accelerationStructures.find(gfxOffset);
            if (iter == m_accelerationStructures.end()) return nullptr;
            return iter->second;
        }
        else
        {
            AP_ERROR("Error trying to get an acceleration structure from a non acceleration structure variable.");
            return nullptr;
        }
    }

    auto ParameterBlock::setSampler(std::string_view name, core::ref<Sampler> pSampler) -> void
    {
        getRootVariable()[name].setSampler(pSampler);
    }

    auto ParameterBlock::setSampler(BindLocation const& bindLocation, core::ref<Sampler> pSampler) -> void
    {
        if (isSamplerType(bindLocation.getType()))
        {
            rhi::ShaderOffset gfxOffset = getGFXShaderOffset(bindLocation);
            auto const& pBoundSampler = pSampler ? pSampler : mp_device->getDefaultSampler();
            m_samplers[gfxOffset] = pBoundSampler;
            m_shaderObject->setBinding(gfxOffset, pBoundSampler->getGfxSamplerState());
        }
        else
        {
            AP_ERROR("Error trying to bind a sampler to a non sampler variable.");
        }
    }

    auto ParameterBlock::getSampler(std::string_view name) const -> core::ref<Sampler>
    {
        return getRootVariable()[name].getSampler();
    }

    auto ParameterBlock::getSampler(BindLocation const& bindLocation) const -> core::ref<Sampler>
    {
        if (isSamplerType(bindLocation.getType()))
        {
            rhi::ShaderOffset gfxOffset = getGFXShaderOffset(bindLocation);
            auto iter = m_samplers.find(gfxOffset);
            if (iter == m_samplers.end()) return nullptr;
            return iter->second;
        }
        else
        {
            AP_ERROR("Error trying to get a sampler from a non sampler variable.");
            return nullptr;
        }
    }

    auto ParameterBlock::setParameterBlock(std::string_view name, core::ref<ParameterBlock> pBlock) -> void
    {
        getRootVariable()[name].setParameterBlock(pBlock);
    }

    auto ParameterBlock::setParameterBlock(BindLocation const& bindLocation, core::ref<ParameterBlock> pBlock) -> void
    {
        if (isParameterBlockType(bindLocation.getType()))
        {
            auto gfxOffset = getGFXShaderOffset(bindLocation);
            m_parameterBlocks[gfxOffset] = pBlock;
            m_shaderObject->setObject(gfxOffset, pBlock ? pBlock->m_shaderObject.get() : nullptr);
        }
        else
        {
            AP_ERROR("Error trying to bind a parameter block to a non parameter block variable.");
        }
    }

    auto ParameterBlock::getParameterBlock(std::string_view name) const -> core::ref<ParameterBlock>
    {
        return getRootVariable()[name].getParameterBlock();
    }

    auto ParameterBlock::getParameterBlock(BindLocation const& bindLocation) const -> core::ref<ParameterBlock>
    {
        if (isParameterBlockType(bindLocation.getType()))
        {
            auto gfxOffset = getGFXShaderOffset(bindLocation);
            auto iter = m_parameterBlocks.find(gfxOffset);
            if (iter == m_parameterBlocks.end()) return nullptr;
            return iter->second;
        }
        else
        {
            AP_ERROR("Error trying to get a parameter block from a non parameter block variable.");
            return nullptr;
        }
    }

    auto ParameterBlock::getSize() const -> size_t
    {
        return m_shaderObject->getSize();
    }

    auto ParameterBlock::updateSpecialization() const -> bool
    {
        return true;
    }

    auto ParameterBlock::prepareDescriptorSets(CommandContext* pCommandContext) -> bool
    {
        for (auto& srv : m_srvs)
        {
            prepareResource(pCommandContext, srv.second ? srv.second->getResource() : nullptr, false);
        }
        for (auto& uav : m_uavs)
        {
            prepareResource(pCommandContext, uav.second ? uav.second->getResource() : nullptr, true);
        }
        for (auto& subObj : m_parameterBlocks)
        {
            subObj.second->prepareDescriptorSets(pCommandContext);
        }
        return true;
    }

    auto ParameterBlock::collectSpecializationArgs(SpecializationArgs& ioArgs) const -> void {}

    auto ParameterBlock::getRawData() const -> void const*
    {
        return m_shaderObject->getRawData();
    }

} // namespace april::graphics
