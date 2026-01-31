// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "shader-variable.hpp"
#include "rhi/parameter-block.hpp"
#include "rhi/buffer.hpp"
#include "rhi/texture.hpp"
#include "rhi/sampler.hpp"
#include "rhi/ray-tracing-acceleration-structure.hpp"
#include "rhi/resource-views.hpp"

#include <core/log/logger.hpp>
#include <core/error/assert.hpp>

namespace april::graphics
{
    ShaderVariable::ShaderVariable(ParameterBlock* pObject, TypedShaderVariableOffset const& m_offset)
        : m_parameterBlock(pObject), m_offset(m_offset)
    {}

    ShaderVariable::ShaderVariable(ParameterBlock* pObject)
        : m_parameterBlock(pObject), m_offset(pObject->getElementType().get(), ShaderVariableOffset::Zero{})
    {}

    // Navigation

    auto ShaderVariable::operator[](std::string_view name) const -> ShaderVariable
    {
        AP_ASSERT(isValid(), "Cannot lookup on invalid ShaderVar.");
        auto result = findMember(name);
        if (!result.isValid())
        {
            AP_ERROR("No member named '{}' found.", name);
        }

        return result;
    }

    auto ShaderVariable::operator[](size_t index) const -> ShaderVariable
    {
        AP_ASSERT(isValid(), "Cannot lookup on invalid ShaderVar.");

        ReflectionType const* pType = getType();

        if (auto pResourceType = pType->asResourceType())
        {
            switch (pResourceType->getType())
            {
            case ReflectionResourceType::Type::ConstantBuffer:
                return getParameterBlock()->getRootVariable()[index];
            default:
                break;
            }
        }

        if (auto pArrayType = pType->asArrayType())
        {
            auto elementCount = pArrayType->getElementCount();
            if (!elementCount || index < elementCount)
            {
                UniformShaderVariableOffset elementUniformLocation = m_offset.getUniform() + index * pArrayType->getElementByteStride();
                ResourceShaderVariableOffset elementResourceLocation(
                    m_offset.getResource().getRangeIndex(),
                    m_offset.getResource().getArrayIndex() * elementCount + ResourceShaderVariableOffset::ArrayIndex(index)
                );
                TypedShaderVariableOffset newOffset =
                    TypedShaderVariableOffset(pArrayType->getElementType().get(), ShaderVariableOffset(elementUniformLocation, elementResourceLocation));
                return ShaderVariable(m_parameterBlock, newOffset);
            }
        }
        else if (auto pStructType = pType->asStructType())
        {
            if (index < pStructType->getMemberCount())
            {
                auto pMember = pStructType->getMember((uint32_t)index);
                TypedShaderVariableOffset newOffset = TypedShaderVariableOffset(pMember->getType(), m_offset + pMember->getBindLocation());
                return ShaderVariable(m_parameterBlock, newOffset);
            }
        }

        AP_CRITICAL("No element or member found at index {}", index);
        return {};
    }

    auto ShaderVariable::findMember(std::string_view name) const -> ShaderVariable
    {
        if (!isValid()) return *this;

        ReflectionType const* pType = getType();

        // If the user is applying `[]` to a `ShaderVar`
        // that represents a constant buffer (or parameter block)
        // then we assume they mean to look up a member
        // inside the buffer/block, and thus implicitly
        // dereference this `ShaderVar`.
        //
        if (auto pResourceType = pType->asResourceType())
        {
            switch (pResourceType->getType())
            {
            case ReflectionResourceType::Type::ConstantBuffer:
                return getParameterBlock()->getRootVariable().findMember(name);
            default:
                break;
            }
        }

        if (auto pStructType = pType->asStructType())
        {
            if (auto pMember = pStructType->getMember(name))
            {
                TypedShaderVariableOffset newOffset = TypedShaderVariableOffset(pMember->getType(), m_offset + pMember->getBindLocation());
                return ShaderVariable(m_parameterBlock, newOffset);
            }
        }

        return {};
    }

    auto ShaderVariable::findMember(uint32_t index) const -> ShaderVariable
    {
        if (!isValid()) return *this;

        ReflectionType const* pType = getType();

        if (auto pResourceType = pType->asResourceType())
        {
            switch (pResourceType->getType())
            {
            case ReflectionResourceType::Type::ConstantBuffer:
                return getParameterBlock()->getRootVariable().findMember(index);
            default:
                break;
            }
        }

        if (auto pStructType = pType->asStructType())
        {
            if (index < pStructType->getMemberCount())
            {
                auto pMember = pStructType->getMember(index);
                TypedShaderVariableOffset newOffset = TypedShaderVariableOffset(pMember->getType(), m_offset + pMember->getBindLocation());
                return ShaderVariable(m_parameterBlock, newOffset);
            }
        }

        return {};
    }

    // Variable assignment

    auto ShaderVariable::setBlob(void const* data, size_t size) const -> void
    {
        ReflectionType const* pType = getType();
        if (auto pResourceType = pType->asResourceType())
        {
            switch (pResourceType->getType())
            {
            case ReflectionResourceType::Type::ConstantBuffer:
                return getParameterBlock()->getRootVariable().setBlob(data, size);
            default:
                break;
            }
        }

        m_parameterBlock->setBlob(data, m_offset, size);
    }

    // Resource binding

    auto ShaderVariable::setBuffer(core::ref<Buffer> pBuffer) const -> void
    {
        m_parameterBlock->setBuffer(m_offset, pBuffer);
    }

    auto ShaderVariable::getBuffer() const -> core::ref<Buffer>
    {
        return m_parameterBlock->getBuffer(m_offset);
    }

    auto ShaderVariable::setTexture(core::ref<Texture> pTexture) const -> void
    {
        m_parameterBlock->setTexture(m_offset, pTexture);
    }

    auto ShaderVariable::getTexture() const -> core::ref<Texture>
    {
        return m_parameterBlock->getTexture(m_offset);
    }

    auto ShaderVariable::setSrv(core::ref<ResourceView> pSrv) const -> void
    {
        m_parameterBlock->setSrv(m_offset, pSrv);
    }

    auto ShaderVariable::getSrv() const -> core::ref<ResourceView>
    {
        return m_parameterBlock->getSrv(m_offset);
    }

    auto ShaderVariable::setUav(core::ref<ResourceView> pUav) const -> void
    {
        m_parameterBlock->setUav(m_offset, pUav);
    }

    auto ShaderVariable::getUav() const -> core::ref<ResourceView>
    {
        return m_parameterBlock->getUav(m_offset);
    }

    auto ShaderVariable::setAccelerationStructure(core::ref<RtAccelerationStructure> pAccl) const -> void
    {
        m_parameterBlock->setAccelerationStructure(m_offset, pAccl);
    }

    auto ShaderVariable::getAccelerationStructure() const -> core::ref<RtAccelerationStructure>
    {
        return m_parameterBlock->getAccelerationStructure(m_offset);
    }

    auto ShaderVariable::setSampler(core::ref<Sampler> pSampler) const -> void
    {
        m_parameterBlock->setSampler(m_offset, pSampler);
    }

    auto ShaderVariable::getSampler() const -> core::ref<Sampler>
    {
        return m_parameterBlock->getSampler(m_offset);
    }

    ShaderVariable::operator core::ref<Sampler>() const
    {
        return getSampler();
    }

    auto ShaderVariable::setParameterBlock(core::ref<ParameterBlock> pBlock) const -> void
    {
        m_parameterBlock->setParameterBlock(m_offset, pBlock);
    }

    auto ShaderVariable::getParameterBlock() const -> core::ref<ParameterBlock>
    {
        return m_parameterBlock->getParameterBlock(m_offset);
    }

    // Offset access

    auto ShaderVariable::operator[](TypedShaderVariableOffset const& otherOffset) const -> ShaderVariable
    {
        if (!isValid()) return *this;

        ReflectionType const* pType = getType();

        if (auto pResourceType = pType->asResourceType())
        {
            switch (pResourceType->getType())
            {
            case ReflectionResourceType::Type::ConstantBuffer:
                return getParameterBlock()->getRootVariable()[otherOffset];
            default:
                break;
            }
        }

        return ShaderVariable{m_parameterBlock, TypedShaderVariableOffset(otherOffset.getType(), m_offset + otherOffset)};
    }

    auto ShaderVariable::operator[](UniformShaderVariableOffset const& loc) const -> ShaderVariable
    {
        if (!isValid()) return *this;

        ReflectionType const* pType = getType();

        if (auto pResourceType = pType->asResourceType())
        {
            switch (pResourceType->getType())
            {
            case ReflectionResourceType::Type::ConstantBuffer:
                return getParameterBlock()->getRootVariable()[loc];
            default:
                break;
            }
        }

        auto byteOffset = loc.getByteOffset();
        if (byteOffset == 0) return *this;

        if (auto pArrayType = pType->asArrayType())
        {
            auto pElementType = pArrayType->getElementType();
            // auto elementCount = pArrayType->getElementCount();
            auto elementStride = pArrayType->getElementByteStride();

            auto elementIndex = byteOffset / elementStride;
            auto offsetIntoElement = byteOffset % elementStride;

            TypedShaderVariableOffset elementOffset =
                TypedShaderVariableOffset(pElementType.get(), ShaderVariableOffset(m_offset.getUniform() + elementIndex * elementStride, m_offset.getResource()));
            ShaderVariable elementCursor(m_parameterBlock, elementOffset);
            return elementCursor[UniformShaderVariableOffset(offsetIntoElement)];
        }
        else if (auto pStructType = pType->asStructType())
        {
            // We want to search for a member matching this offset
            //
            // TODO: A binary search should be preferred to the linear
            // search here.
            auto memberCount = pStructType->getMemberCount();
            for (uint32_t m = 0; m < memberCount; ++m)
            {
                auto pMember = pStructType->getMember(m);
                auto memberByteOffset = pMember->getByteOffset();
                auto memberByteSize = pMember->getType()->getByteSize();

                if (byteOffset < memberByteOffset)  continue;
                if (byteOffset >= memberByteOffset + memberByteSize) continue;

                auto offsetIntoMember = byteOffset - memberByteOffset;
                TypedShaderVariableOffset memberOffset = TypedShaderVariableOffset(pMember->getType(), m_offset + pMember->getBindLocation());
                ShaderVariable memberCursor(m_parameterBlock, memberOffset);

                return memberCursor[UniformShaderVariableOffset(offsetIntoMember)];
            }
        }

        AP_ERROR("No element or member found at m_offset {}", byteOffset);
        return ShaderVariable();
    }

    auto ShaderVariable::getRawData() const -> void const*
    {
        return (uint8_t*) (m_parameterBlock->getRawData()) + m_offset.getUniform().getByteOffset();
    }

    auto ShaderVariable::setImpl(core::ref<Texture> const& pTexture) const -> void
    {
        setTexture(pTexture);
    }

    auto ShaderVariable::setImpl(core::ref<Sampler> const& pSampler) const -> void
    {
        setSampler(pSampler);
    }

    auto ShaderVariable::setImpl(core::ref<Buffer> const& pBuffer) const -> void
    {
        setBuffer(pBuffer);
    }

    auto ShaderVariable::setImpl(core::ref<ParameterBlock> const& pBlock) const -> void
    {
        setParameterBlock(pBlock);
    }

} // namespace april::graphics
