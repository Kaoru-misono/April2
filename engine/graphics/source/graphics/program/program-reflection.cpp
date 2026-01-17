// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "program-reflection.hpp"
#include "program.hpp"
#include "program-version.hpp"
#include "rhi/render-device.hpp"
// #include "Utils/StringUtils.h"
// #include "Utils/Scripting/ScriptBindings.h"

#include <core/log/logger.hpp>
#include <core/error/assert.hpp>
#include <core/tools/enum.hpp>

#include <slang.h>

#include <map>
#include <algorithm>

using namespace slang;

namespace april::graphics
{
    namespace
    {
        char const* kRootDescriptorAttribute = "root";
    }

    TypedShaderVariableOffset::TypedShaderVariableOffset(ReflectionType const* pType, ShaderVariableOffset offset)
        : ShaderVariableOffset(offset), m_type(pType)
    {}

    auto TypedShaderVariableOffset::operator[](std::string_view name) const -> TypedShaderVariableOffset
    {
        if (!isValid())
            return *this;

        auto pType = getType();

        if (auto pStructType = pType->asStructType())
        {
            if (auto pMember = pStructType->findMember(name))
            {
                return TypedShaderVariableOffset(pMember->getType(), (*this) + pMember->getBindLocation());
            }
        }

        AP_CRITICAL("No member named '{}' found.", name);
        return {};
    }

    auto TypedShaderVariableOffset::operator[](size_t index) const -> TypedShaderVariableOffset
    {
        (void) index;

        throw 99; // Keep original logic
    }

    auto ReflectionType::getZeroOffset() const -> TypedShaderVariableOffset
    {
        return TypedShaderVariableOffset(this, ShaderVariableOffset::Zero::kZero);
    }

    auto ReflectionType::getMemberOffset(std::string_view name) const -> TypedShaderVariableOffset
    {
        return getZeroOffset()[name];
    }

    // Represents one link in a "breadcrumb trail"
    struct ReflectionPathLink
    {
        ReflectionPathLink const* pParent = nullptr;
        VariableLayoutReflection* pVar = nullptr;
    };

    // Represents a full "breadcrumb trail"
    struct ReflectionPath
    {
        ReflectionPathLink* pPrimary = nullptr;
        ReflectionPathLink* pDeferred = nullptr;
    };

    // A helper RAII type to extend a `ReflectionPath`
    struct ExtendedReflectionPath : ReflectionPath
    {
        ExtendedReflectionPath(ReflectionPath const* pParent, VariableLayoutReflection* pVar)
        {
            if (pParent)
            {
                pPrimary = pParent->pPrimary;
                pDeferred = pParent->pDeferred;
            }

            if (pVar)
            {
                primaryLinkStorage.pParent = pPrimary;
                primaryLinkStorage.pVar = pVar;
                pPrimary = &primaryLinkStorage;

                // if (auto pDeferredVar = pVar->getPendingDataLayout())
                // {
                //     deferredLinkStorage.pParent = pDeferred;
                //     deferredLinkStorage.pVar = pDeferredVar;
                //     pDeferred = &deferredLinkStorage;
                // }
            }
        }

        ReflectionPathLink primaryLinkStorage;
        ReflectionPathLink deferredLinkStorage;
    };

    static auto getResourceType(TypeReflection* pSlangType) -> ReflectionResourceType::Type
    {
        switch (pSlangType->unwrapArray()->getKind())
        {
        case TypeReflection::Kind::ParameterBlock:
        case TypeReflection::Kind::ConstantBuffer:
            return ReflectionResourceType::Type::ConstantBuffer;
        case TypeReflection::Kind::SamplerState:
            return ReflectionResourceType::Type::Sampler;
        case TypeReflection::Kind::ShaderStorageBuffer:
            return ReflectionResourceType::Type::StructuredBuffer;
        case TypeReflection::Kind::TextureBuffer:
            return ReflectionResourceType::Type::TypedBuffer;
        case TypeReflection::Kind::Resource:
            switch (pSlangType->getResourceShape() & SLANG_RESOURCE_BASE_SHAPE_MASK)
            {
            case SLANG_STRUCTURED_BUFFER:
                return ReflectionResourceType::Type::StructuredBuffer;
            case SLANG_BYTE_ADDRESS_BUFFER:
                return ReflectionResourceType::Type::RawBuffer;
            case SLANG_TEXTURE_BUFFER:
                return ReflectionResourceType::Type::TypedBuffer;
            case SLANG_ACCELERATION_STRUCTURE:
                return ReflectionResourceType::Type::AccelerationStructure;
            case SLANG_TEXTURE_1D:
            case SLANG_TEXTURE_2D:
            case SLANG_TEXTURE_3D:
            case SLANG_TEXTURE_CUBE:
                return ReflectionResourceType::Type::Texture;
            default:
                AP_UNREACHABLE();
            }
        default:
            AP_UNREACHABLE();
        }
    }

    static auto getShaderAccess(TypeReflection* pSlangType) -> ReflectionResourceType::ShaderAccess
    {
        pSlangType = pSlangType->unwrapArray();

        switch (pSlangType->getKind())
        {
        case TypeReflection::Kind::SamplerState:
        case TypeReflection::Kind::ConstantBuffer:
            return ReflectionResourceType::ShaderAccess::Read;
            break;

        case TypeReflection::Kind::Resource:
        case TypeReflection::Kind::ShaderStorageBuffer:
            switch (pSlangType->getResourceAccess())
            {
            case SLANG_RESOURCE_ACCESS_NONE:
                return ReflectionResourceType::ShaderAccess::Undefined;

            case SLANG_RESOURCE_ACCESS_READ:
                return ReflectionResourceType::ShaderAccess::Read;

            default:
                return ReflectionResourceType::ShaderAccess::ReadWrite;
            }
            break;

        default:
            return ReflectionResourceType::ShaderAccess::Undefined;
        }
    }

    static auto getReturnType(TypeReflection* pType) -> ReflectionResourceType::ReturnType
    {
        if (!pType)
            return ReflectionResourceType::ReturnType::Unknown;

        switch (pType->getScalarType())
        {
        case TypeReflection::ScalarType::Float32:
            return ReflectionResourceType::ReturnType::Float;
        case TypeReflection::ScalarType::Int32:
            return ReflectionResourceType::ReturnType::Int;
        case TypeReflection::ScalarType::UInt32:
            return ReflectionResourceType::ReturnType::Uint;
        case TypeReflection::ScalarType::Float64:
            return ReflectionResourceType::ReturnType::Double;
        case TypeReflection::ScalarType::None:
            return ReflectionResourceType::ReturnType::Unknown;
        default:
            return ReflectionResourceType::ReturnType::Unknown;
        }
    }

    static auto getResourceDimensions(SlangResourceShape shape) -> ReflectionResourceType::Dimensions
    {
        switch (shape)
        {
        case SLANG_TEXTURE_1D:
            return ReflectionResourceType::Dimensions::Texture1D;
        case SLANG_TEXTURE_1D_ARRAY:
            return ReflectionResourceType::Dimensions::Texture1DArray;
        case SLANG_TEXTURE_2D:
            return ReflectionResourceType::Dimensions::Texture2D;
        case SLANG_TEXTURE_2D_ARRAY:
            return ReflectionResourceType::Dimensions::Texture2DArray;
        case SLANG_TEXTURE_2D_MULTISAMPLE:
            return ReflectionResourceType::Dimensions::Texture2DMS;
        case SLANG_TEXTURE_2D_MULTISAMPLE_ARRAY:
            return ReflectionResourceType::Dimensions::Texture2DMSArray;
        case SLANG_TEXTURE_3D:
            return ReflectionResourceType::Dimensions::Texture3D;
        case SLANG_TEXTURE_CUBE:
            return ReflectionResourceType::Dimensions::TextureCube;
        case SLANG_TEXTURE_CUBE_ARRAY:
            return ReflectionResourceType::Dimensions::TextureCubeArray;
        case SLANG_ACCELERATION_STRUCTURE:
            return ReflectionResourceType::Dimensions::AccelerationStructure;

        case SLANG_TEXTURE_BUFFER:
        case SLANG_STRUCTURED_BUFFER:
        case SLANG_BYTE_ADDRESS_BUFFER:
            return ReflectionResourceType::Dimensions::Buffer;

        default:
            return ReflectionResourceType::Dimensions::Unknown;
        }
    }

    auto getVariableType(TypeReflection::ScalarType slangScalarType, uint32_t rows, uint32_t columns) -> ReflectionBasicType::Type
    {
        switch (slangScalarType)
        {
        case TypeReflection::ScalarType::Bool:
            AP_ASSERT(rows == 1);
            switch (columns)
            {
            case 1: return ReflectionBasicType::Type::Bool;
            case 2: return ReflectionBasicType::Type::Bool2;
            case 3: return ReflectionBasicType::Type::Bool3;
            case 4: return ReflectionBasicType::Type::Bool4;
            }
            break;
        case TypeReflection::ScalarType::UInt8:
            AP_ASSERT(rows == 1);
            switch (columns)
            {
            case 1: return ReflectionBasicType::Type::Uint8;
            case 2: return ReflectionBasicType::Type::Uint8_2;
            case 3: return ReflectionBasicType::Type::Uint8_3;
            case 4: return ReflectionBasicType::Type::Uint8_4;
            }
            break;
        case TypeReflection::ScalarType::UInt16:
            AP_ASSERT(rows == 1);
            switch (columns)
            {
            case 1: return ReflectionBasicType::Type::Uint16;
            case 2: return ReflectionBasicType::Type::Uint16_2;
            case 3: return ReflectionBasicType::Type::Uint16_3;
            case 4: return ReflectionBasicType::Type::Uint16_4;
            }
            break;
        case TypeReflection::ScalarType::UInt32:
            AP_ASSERT(rows == 1);
            switch (columns)
            {
            case 1: return ReflectionBasicType::Type::Uint;
            case 2: return ReflectionBasicType::Type::Uint2;
            case 3: return ReflectionBasicType::Type::Uint3;
            case 4: return ReflectionBasicType::Type::Uint4;
            }
            break;
        case TypeReflection::ScalarType::UInt64:
            AP_ASSERT(rows == 1);
            switch (columns)
            {
            case 1: return ReflectionBasicType::Type::Uint64;
            case 2: return ReflectionBasicType::Type::Uint64_2;
            case 3: return ReflectionBasicType::Type::Uint64_3;
            case 4: return ReflectionBasicType::Type::Uint64_4;
            }
            break;
        case TypeReflection::ScalarType::Int8:
            AP_ASSERT(rows == 1);
            switch (columns)
            {
            case 1: return ReflectionBasicType::Type::Int8;
            case 2: return ReflectionBasicType::Type::Int8_2;
            case 3: return ReflectionBasicType::Type::Int8_3;
            case 4: return ReflectionBasicType::Type::Int8_4;
            }
            break;
        case TypeReflection::ScalarType::Int16:
            AP_ASSERT(rows == 1);
            switch (columns)
            {
            case 1: return ReflectionBasicType::Type::Int16;
            case 2: return ReflectionBasicType::Type::Int16_2;
            case 3: return ReflectionBasicType::Type::Int16_3;
            case 4: return ReflectionBasicType::Type::Int16_4;
            }
            break;
        case TypeReflection::ScalarType::Int32:
            AP_ASSERT(rows == 1);
            switch (columns)
            {
            case 1: return ReflectionBasicType::Type::Int;
            case 2: return ReflectionBasicType::Type::Int2;
            case 3: return ReflectionBasicType::Type::Int3;
            case 4: return ReflectionBasicType::Type::Int4;
            }
            break;
        case TypeReflection::ScalarType::Int64:
            AP_ASSERT(rows == 1);
            switch (columns)
            {
            case 1: return ReflectionBasicType::Type::Int64;
            case 2: return ReflectionBasicType::Type::Int64_2;
            case 3: return ReflectionBasicType::Type::Int64_3;
            case 4: return ReflectionBasicType::Type::Int64_4;
            }
            break;
        case TypeReflection::ScalarType::Float16:
            switch (rows)
            {
            case 1:
                switch (columns)
                {
                case 1: return ReflectionBasicType::Type::Float16;
                case 2: return ReflectionBasicType::Type::Float16_2;
                case 3: return ReflectionBasicType::Type::Float16_3;
                case 4: return ReflectionBasicType::Type::Float16_4;
                }
                break;
            case 2:
                switch (columns)
                {
                case 2: return ReflectionBasicType::Type::Float16_2x2;
                case 3: return ReflectionBasicType::Type::Float16_2x3;
                case 4: return ReflectionBasicType::Type::Float16_2x4;
                }
                break;
            case 3:
                switch (columns)
                {
                case 2: return ReflectionBasicType::Type::Float16_3x2;
                case 3: return ReflectionBasicType::Type::Float16_3x3;
                case 4: return ReflectionBasicType::Type::Float16_3x4;
                }
                break;
            case 4:
                switch (columns)
                {
                case 2: return ReflectionBasicType::Type::Float16_4x2;
                case 3: return ReflectionBasicType::Type::Float16_4x3;
                case 4: return ReflectionBasicType::Type::Float16_4x4;
                }
                break;
            }
            break;
        case TypeReflection::ScalarType::Float32:
            switch (rows)
            {
            case 1:
                switch (columns)
                {
                case 1: return ReflectionBasicType::Type::Float;
                case 2: return ReflectionBasicType::Type::Float2;
                case 3: return ReflectionBasicType::Type::Float3;
                case 4: return ReflectionBasicType::Type::Float4;
                }
                break;
            case 2:
                switch (columns)
                {
                case 2: return ReflectionBasicType::Type::Float2x2;
                case 3: return ReflectionBasicType::Type::Float2x3;
                case 4: return ReflectionBasicType::Type::Float2x4;
                }
                break;
            case 3:
                switch (columns)
                {
                case 2: return ReflectionBasicType::Type::Float3x2;
                case 3: return ReflectionBasicType::Type::Float3x3;
                case 4: return ReflectionBasicType::Type::Float3x4;
                }
                break;
            case 4:
                switch (columns)
                {
                case 2: return ReflectionBasicType::Type::Float4x2;
                case 3: return ReflectionBasicType::Type::Float4x3;
                case 4: return ReflectionBasicType::Type::Float4x4;
                }
                break;
            }
            break;
        case TypeReflection::ScalarType::Float64:
            AP_ASSERT(rows == 1);
            switch (columns)
            {
            case 1: return ReflectionBasicType::Type::Float64;
            case 2: return ReflectionBasicType::Type::Float64_2;
            case 3: return ReflectionBasicType::Type::Float64_3;
            case 4: return ReflectionBasicType::Type::Float64_4;
            }
            break;
        default:
            AP_UNREACHABLE();
        }

        AP_UNREACHABLE();
    }

    static auto getStructuredBufferType(TypeReflection* pSlangType) -> ReflectionResourceType::StructuredType
    {
        auto invalid = ReflectionResourceType::StructuredType::Invalid;

        if (pSlangType->getKind() != TypeReflection::Kind::Resource)
            return invalid; // not a structured buffer

        if (pSlangType->getResourceShape() != SLANG_STRUCTURED_BUFFER)
            return invalid; // not a structured buffer

        switch (pSlangType->getResourceAccess())
        {
        case SLANG_RESOURCE_ACCESS_READ:
            return ReflectionResourceType::StructuredType::Default;

        case SLANG_RESOURCE_ACCESS_READ_WRITE:
        case SLANG_RESOURCE_ACCESS_RASTER_ORDERED:
            return ReflectionResourceType::StructuredType::Counter;
        case SLANG_RESOURCE_ACCESS_APPEND:
            return ReflectionResourceType::StructuredType::Append;
        case SLANG_RESOURCE_ACCESS_CONSUME:
            return ReflectionResourceType::StructuredType::Consume;
        default:
            AP_UNREACHABLE();
        }
    }

    // Forward declarations
    auto reflectVariable(
        VariableLayoutReflection* pSlangLayout,
        ShaderVariableOffset::RangeIndex rangeIndex,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ProgramVersion const* pProgramVersion
    ) -> core::ref<ReflectionVariable>;

    auto reflectType(
        TypeLayoutReflection* pSlangType,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ProgramVersion const* pProgramVersion
    ) -> core::ref<ReflectionType>;

    static auto hasUsage(slang::TypeLayoutReflection* pSlangTypeLayout, SlangParameterCategory resourceKind) -> bool
    {
        auto kindCount = pSlangTypeLayout->getCategoryCount();
        for (unsigned int ii = 0; ii < kindCount; ++ii)
        {
            if (SlangParameterCategory(pSlangTypeLayout->getCategoryByIndex(ii)) == resourceKind)
                return true;
        }
        return false;
    }

    static auto getRegisterIndexFromPath(ReflectionPathLink const* pPath, SlangParameterCategory category) -> size_t
    {
        uint32_t offset = 0;
        for (auto pp = pPath; pp; pp = pp->pParent)
        {
            if (pp->pVar)
            {
                if (pp->pVar->getType()->getKind() == slang::TypeReflection::Kind::ParameterBlock &&
                    hasUsage(pp->pVar->getTypeLayout(), SLANG_PARAMETER_CATEGORY_REGISTER_SPACE) &&
                    category != SLANG_PARAMETER_CATEGORY_REGISTER_SPACE)
                {
                    return offset;
                }

                offset += (uint32_t)pp->pVar->getOffset(category);
                continue;
            }
            AP_CRITICAL("Invalid reflection path");
        }
        return offset;
    }

    static auto getRegisterSpaceFromPath(ReflectionPathLink const* pPath, SlangParameterCategory category) -> uint32_t
    {
        uint32_t offset = 0;
        for (auto pp = pPath; pp; pp = pp->pParent)
        {
            if (pp->pVar)
            {
                if (pp->pVar->getTypeLayout()->getKind() == slang::TypeReflection::Kind::ParameterBlock)
                {
                    return offset + (uint32_t)getRegisterIndexFromPath(pp, SLANG_PARAMETER_CATEGORY_REGISTER_SPACE);
                }
                offset += (uint32_t)pp->pVar->getBindingSpace(category);
                continue;
            }

            AP_CRITICAL("Invalid reflection path");
        }
        return offset;
    }

    static auto getShaderResourceType(ReflectionResourceType const* pType) -> ShaderResourceType
    {
        auto shaderAccess = pType->getShaderAccess();
        switch (pType->getType())
        {
        case ReflectionResourceType::Type::ConstantBuffer:
            return ShaderResourceType::ConstantBuffer; // Cbv
        case ReflectionResourceType::Type::Texture:
            return shaderAccess == ReflectionResourceType::ShaderAccess::Read ? ShaderResourceType::TextureSrv : ShaderResourceType::TextureUav;
        case ReflectionResourceType::Type::RawBuffer:
            return shaderAccess == ReflectionResourceType::ShaderAccess::Read ? ShaderResourceType::RawBufferSrv : ShaderResourceType::RawBufferUav;
        case ReflectionResourceType::Type::StructuredBuffer:
            return shaderAccess == ReflectionResourceType::ShaderAccess::Read ? ShaderResourceType::StructuredBufferSrv : ShaderResourceType::StructuredBufferUav;
        case ReflectionResourceType::Type::TypedBuffer:
            return shaderAccess == ReflectionResourceType::ShaderAccess::Read ? ShaderResourceType::TypedBufferSrv : ShaderResourceType::TypedBufferUav;
        case ReflectionResourceType::Type::AccelerationStructure:
            AP_ASSERT(shaderAccess == ReflectionResourceType::ShaderAccess::Read);
            return ShaderResourceType::AccelerationStructureSrv;
        case ReflectionResourceType::Type::Sampler:
            return ShaderResourceType::Sampler;
        default:
            AP_UNREACHABLE();
        }
    }

    static auto getParameterCategory(TypeLayoutReflection* pTypeLayout) -> ParameterCategory
    {
        ParameterCategory category = pTypeLayout->getParameterCategory();
        if (category == ParameterCategory::Mixed)
        {
            switch (pTypeLayout->getKind())
            {
            case TypeReflection::Kind::ConstantBuffer:
            case TypeReflection::Kind::ParameterBlock:
            case TypeReflection::Kind::None:
                category = ParameterCategory::ConstantBuffer;
                break;
            default:
                break;
            }
        }
        return category;
    }

    // static auto getParameterCategory(VariableLayoutReflection* pVarLayout) -> ParameterCategory
    // {
    //     return getParameterCategory(pVarLayout->getTypeLayout());
    // }

    static auto extractDefaultConstantBufferBinding(
        slang::TypeLayoutReflection* pSlangType,
        ReflectionPath* pPath,
        ParameterBlockReflection* pBlock,
        bool shouldUseRootConstants
    ) -> void
    {
        auto pContainerLayout = pSlangType->getContainerVarLayout();
        AP_ASSERT(pContainerLayout, "Container layout is null");

        ExtendedReflectionPath containerPath(pPath, pContainerLayout);
        int32_t containerCategoryCount = pContainerLayout->getCategoryCount();
        for (int32_t containerCategoryIndex = 0; containerCategoryIndex < containerCategoryCount; ++containerCategoryIndex)
        {
            auto containerCategory = pContainerLayout->getCategoryByIndex(containerCategoryIndex);
            switch (containerCategory)
            {
            case slang::ParameterCategory::DescriptorTableSlot:
            case slang::ParameterCategory::ConstantBuffer:
            {
                ParameterBlockReflection::DefaultConstantBufferBindingInfo defaultConstantBufferInfo;
                defaultConstantBufferInfo.regIndex =
                    (uint32_t)getRegisterIndexFromPath(containerPath.pPrimary, SlangParameterCategory(containerCategory));
                defaultConstantBufferInfo.regSpace =
                    getRegisterSpaceFromPath(containerPath.pPrimary, SlangParameterCategory(containerCategory));
                defaultConstantBufferInfo.useRootConstants = shouldUseRootConstants;
                pBlock->setDefaultConstantBufferBindingInfo(defaultConstantBufferInfo);
            }
            break;

            default:
                break;
            }
        }
    }

    auto reflectResourceType(
        TypeLayoutReflection* pSlangType,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ProgramVersion const* pProgramVersion
    ) -> core::ref<ReflectionType>
    {
        ReflectionResourceType::Type type = getResourceType(pSlangType->getType());
        ReflectionResourceType::Dimensions dims = getResourceDimensions(pSlangType->getResourceShape());
        ReflectionResourceType::ShaderAccess shaderAccess = getShaderAccess(pSlangType->getType());
        ReflectionResourceType::ReturnType retType = getReturnType(pSlangType->getType());
        ReflectionResourceType::StructuredType structuredType = getStructuredBufferType(pSlangType->getType());

        AP_ASSERT(pPath->pPrimary && pPath->pPrimary->pVar, "Invalid reflection path");
        std::string name(pPath->pPrimary->pVar->getName());

        auto pVar = pPath->pPrimary->pVar->getVariable();
        bool isRootDescriptor =
            pVar->findUserAttributeByName(pProgramVersion->getSlangSession()->getGlobalSession(), kRootDescriptorAttribute) != nullptr;

        if (isRootDescriptor)
        {
            if (type != ReflectionResourceType::Type::RawBuffer && type != ReflectionResourceType::Type::StructuredBuffer &&
                type != ReflectionResourceType::Type::AccelerationStructure)
            {
                AP_CRITICAL(
                    "Resource '{}' cannot be bound as root descriptor. Only raw buffers, structured buffers, and acceleration structures are "
                    "supported.",
                    name
                );
            }
            if (shaderAccess != ReflectionResourceType::ShaderAccess::Read && shaderAccess != ReflectionResourceType::ShaderAccess::ReadWrite)
            {
                AP_CRITICAL("Buffer '{}' cannot be bound as root descriptor. Only SRV/UAVs are supported.", name);
            }
            AP_ASSERT(
                type != ReflectionResourceType::Type::AccelerationStructure || shaderAccess == ReflectionResourceType::ShaderAccess::Read,
                "Acceleration structures must be read-only"
            );

            if (type == ReflectionResourceType::Type::StructuredBuffer)
            {
                AP_ASSERT(structuredType != ReflectionResourceType::StructuredType::Invalid, "Invalid structured buffer type");
                if (structuredType == ReflectionResourceType::StructuredType::Append ||
                    structuredType == ReflectionResourceType::StructuredType::Consume)
                {
                    AP_CRITICAL(
                        "StructuredBuffer '{}' cannot be bound as root descriptor. Only regular structured buffers are supported, not "
                        "append/consume buffers.",
                        name
                    );
                }
            }
            AP_ASSERT(
                dims == ReflectionResourceType::Dimensions::Buffer || dims == ReflectionResourceType::Dimensions::AccelerationStructure,
                "Invalid dimensions for root descriptor"
            );
        }

        auto pType = ReflectionResourceType::create(type, dims, structuredType, retType, shaderAccess, pSlangType);

        ParameterCategory category = getParameterCategory(pSlangType);
        ParameterBlockReflection::ResourceRangeBindingInfo bindingInfo;
        bindingInfo.regIndex = (uint32_t)getRegisterIndexFromPath(pPath->pPrimary, SlangParameterCategory(category));
        bindingInfo.regSpace = getRegisterSpaceFromPath(pPath->pPrimary, SlangParameterCategory(category));
        bindingInfo.dimension = dims;

        if (isRootDescriptor)
            bindingInfo.flavor = ParameterBlockReflection::ResourceRangeBindingInfo::Flavor::RootDescriptor;

        switch (type)
        {
        default:
            break;

        case ReflectionResourceType::Type::StructuredBuffer:
        {
            const auto& pElementLayout = pSlangType->getElementTypeLayout();
            auto pBufferType = reflectType(pElementLayout, pBlock, pPath, pProgramVersion);
            pType->setStructType(pBufferType);
        }
        break;

        case ReflectionResourceType::Type::ConstantBuffer:
        {
            auto pSubBlock = ParameterBlockReflection::createEmpty(pProgramVersion);
            const auto& pElementLayout = pSlangType->getElementTypeLayout();
            auto pElementType = reflectType(pElementLayout, pSubBlock.get(), pPath, pProgramVersion);
            pSubBlock->setElementType(pElementType);

            extractDefaultConstantBufferBinding(pSlangType, pPath, pSubBlock.get(), /*shouldUseRootConstants:*/ false);

            pSubBlock->finalize();

            pType->setStructType(pElementType);
            pType->setParameterBlockReflector(pSubBlock);

            if (pSlangType->getKind() == slang::TypeReflection::Kind::ParameterBlock)
            {
                bindingInfo.flavor = ParameterBlockReflection::ResourceRangeBindingInfo::Flavor::ParameterBlock;
            }
            else
            {
                bindingInfo.flavor = ParameterBlockReflection::ResourceRangeBindingInfo::Flavor::ConstantBuffer;
            }
            bindingInfo.subObjectReflector = pSubBlock;
        }
        break;
        }

        if (pBlock)
        {
            pBlock->addResourceRange(bindingInfo);
        }

        return pType;
    }

    auto reflectStructType(
        TypeLayoutReflection* pSlangType,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ProgramVersion const* pProgramVersion
    ) -> core::ref<ReflectionType>
    {
        auto pSlangName = pSlangType->getName();
        auto name = pSlangName ? std::string(pSlangName) : std::string();

        auto pType = ReflectionStructType::create(pSlangType->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM), name, pSlangType);

        ReflectionStructType::BuildState buildState;

        for (uint32_t i = 0; i < pSlangType->getFieldCount(); i++)
        {
            auto pSlangField = pSlangType->getFieldByIndex(i);
            ExtendedReflectionPath fieldPath(pPath, pSlangField);

            auto pVar = reflectVariable(pSlangField, pType->getResourceRangeCount(), pBlock, &fieldPath, pProgramVersion);
            if (pVar)
                pType->addMember(pVar, buildState);
        }
        return pType;
    }

    static auto getByteSize(TypeLayoutReflection* pSlangType) -> size_t
    {
        return pSlangType->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM);
    }

    auto reflectArrayType(
        TypeLayoutReflection* pSlangType,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ProgramVersion const* pProgramVersion
    ) -> core::ref<ReflectionType>
    {
        uint32_t elementCount = (uint32_t)pSlangType->getElementCount();
        uint32_t elementByteStride = (uint32_t)pSlangType->getElementStride(SLANG_PARAMETER_CATEGORY_UNIFORM);

        auto pElementType = reflectType(pSlangType->getElementTypeLayout(), pBlock, pPath, pProgramVersion);
        auto pArrayType =
            ReflectionArrayType::create(elementCount, elementByteStride, pElementType, getByteSize(pSlangType), pSlangType);
        return pArrayType;
    }

    auto reflectBasicType(TypeLayoutReflection* pSlangType) -> core::ref<ReflectionType>
    {
        const bool isRowMajor = pSlangType->getMatrixLayoutMode() == SLANG_MATRIX_LAYOUT_ROW_MAJOR;
        auto type = getVariableType(pSlangType->getScalarType(), pSlangType->getRowCount(), pSlangType->getColumnCount());
        auto pType = ReflectionBasicType::create(type, isRowMajor, pSlangType->getSize(), pSlangType);
        return pType;
    }

    auto reflectInterfaceType(
        TypeLayoutReflection* pSlangType,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ProgramVersion const* pProgramVersion
    ) -> core::ref<ReflectionType>
    {
        (void) pProgramVersion;

        auto pType = ReflectionInterfaceType::create(pSlangType);

        ParameterCategory category = getParameterCategory(pSlangType);
        ParameterBlockReflection::ResourceRangeBindingInfo bindingInfo;
        bindingInfo.regIndex = (uint32_t)getRegisterIndexFromPath(pPath->pPrimary, SlangParameterCategory(category));
        bindingInfo.regSpace = getRegisterSpaceFromPath(pPath->pPrimary, SlangParameterCategory(category));

        bindingInfo.flavor = ParameterBlockReflection::ResourceRangeBindingInfo::Flavor::Interface;

        // if (auto pSlangPendingTypeLayout = pSlangType->getPendingDataTypeLayout())
        // {
        //     ReflectionPath subPath;
        //     subPath.pPrimary = pPath->pDeferred;
        //     subPath.pDeferred = nullptr;

        //     auto pPendingBlock = ParameterBlockReflection::createEmpty(pProgramVersion);
        //     auto pPendingType = reflectType(pSlangPendingTypeLayout, pPendingBlock.get(), &subPath, pProgramVersion);
        //     pPendingBlock->setElementType(pPendingType);

        //     // TODO: What to do if `pPendingType->getByteSize()` is non-zero?

        //     pPendingBlock->finalize();

        //     pType->setParameterBlockReflector(pPendingBlock);

        //     bindingInfo.subObjectReflector = pPendingBlock;

        //     category = slang::ParameterCategory::Uniform;
        //     bindingInfo.regIndex = (uint32_t)getRegisterIndexFromPath(pPath->pDeferred, SlangParameterCategory(category));
        //     bindingInfo.regSpace = getRegisterSpaceFromPath(pPath->pPrimary, SlangParameterCategory(category));
        // }

        if (pBlock)
        {
            pBlock->addResourceRange(bindingInfo);
        }

        return pType;
    }

    auto reflectSpecializedType(
        TypeLayoutReflection* pSlangType,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ProgramVersion const* pProgramVersion
    ) -> core::ref<ReflectionType>
    {
        auto pSlangBaseType = pSlangType->getElementTypeLayout();

        // auto pSlangVarLayout = pSlangType->getSpecializedTypePendingDataVarLayout();

        ReflectionPathLink deferredLink;
        deferredLink.pParent = pPath->pPrimary;
        // deferredLink.pVar = pSlangVarLayout;

        ReflectionPath path;
        path.pPrimary = pPath->pPrimary;
        path.pDeferred = &deferredLink;

        return reflectType(pSlangBaseType, pBlock, &path, pProgramVersion);
    }

    auto reflectType(
        TypeLayoutReflection* pSlangType,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ProgramVersion const* pProgramVersion
    ) -> core::ref<ReflectionType>
    {
        AP_ASSERT(pSlangType, "Slang type is null");
        auto kind = pSlangType->getType()->getKind();
        switch (kind)
        {
        case TypeReflection::Kind::ParameterBlock:
        case TypeReflection::Kind::Resource:
        case TypeReflection::Kind::SamplerState:
        case TypeReflection::Kind::ConstantBuffer:
        case TypeReflection::Kind::ShaderStorageBuffer:
        case TypeReflection::Kind::TextureBuffer:
            return reflectResourceType(pSlangType, pBlock, pPath, pProgramVersion);
        case TypeReflection::Kind::Struct:
            return reflectStructType(pSlangType, pBlock, pPath, pProgramVersion);
        case TypeReflection::Kind::Array:
            return reflectArrayType(pSlangType, pBlock, pPath, pProgramVersion);
        case TypeReflection::Kind::Interface:
            return reflectInterfaceType(pSlangType, pBlock, pPath, pProgramVersion);
        case TypeReflection::Kind::Specialized:
            return reflectSpecializedType(pSlangType, pBlock, pPath, pProgramVersion);
        case TypeReflection::Kind::Scalar:
        case TypeReflection::Kind::Matrix:
        case TypeReflection::Kind::Vector:
            return reflectBasicType(pSlangType);
        case TypeReflection::Kind::None:
            return nullptr;
        case TypeReflection::Kind::GenericTypeParameter:
            AP_CRITICAL("Unexpected Slang type");
        default:
            AP_UNREACHABLE();
        }
    }

    auto reflectVariable(
        VariableLayoutReflection* pSlangLayout,
        ShaderVariableOffset::RangeIndex rangeIndex,
        ParameterBlockReflection* pBlock,
        ReflectionPath* pPath,
        ProgramVersion const* pProgramVersion
    ) -> core::ref<ReflectionVariable>
    {
        AP_ASSERT(pPath, "Reflection path is null");
        std::string name(pSlangLayout->getName());

        auto pType = reflectType(pSlangLayout->getTypeLayout(), pBlock, pPath, pProgramVersion);
        auto byteOffset = (ShaderVariableOffset::ByteOffset)pSlangLayout->getOffset(SLANG_PARAMETER_CATEGORY_UNIFORM);

        auto pVar =
            ReflectionVariable::create(name, pType, ShaderVariableOffset(UniformShaderVariableOffset(byteOffset), ResourceShaderVariableOffset(rangeIndex, 0)));

        return pVar;
    }

    auto reflectTopLevelVariable(
        VariableLayoutReflection* pSlangLayout,
        ShaderVariableOffset::RangeIndex rangeIndex,
        ParameterBlockReflection* pBlock,
        ProgramVersion const* pProgramVersion
    ) -> core::ref<ReflectionVariable>
    {
        ExtendedReflectionPath path(nullptr, pSlangLayout);

        return reflectVariable(pSlangLayout, rangeIndex, pBlock, &path, pProgramVersion);
    }

    static void storeShaderVariable(
        ReflectionPath const& path,
        SlangParameterCategory category,
        std::string const& name,
        ProgramReflection::VariableMap& varMap,
        ProgramReflection::VariableMap* pVarMapBySemantic,
        uint32_t count,
        uint32_t stride
    )
    {
        auto pVar = path.pPrimary->pVar;

        ProgramReflection::ShaderVariable var;
        const auto& pTypeLayout = pVar->getTypeLayout();
        var.type = getVariableType(pTypeLayout->getScalarType(), pTypeLayout->getRowCount(), pTypeLayout->getColumnCount());

        uint32_t baseIndex = (uint32_t)getRegisterIndexFromPath(path.pPrimary, category);
        for (uint32_t i = 0; i < std::max(count, 1u); i++)
        {
            var.bindLocation = baseIndex + (i * stride);
            var.semanticName = pVar->getSemanticName();
            if (count)
            {
                var.semanticName += '[' + std::to_string(i) + ']';
            }
            varMap[name] = var;
            if (pVarMapBySemantic)
            {
                (*pVarMapBySemantic)[var.semanticName] = var;
            }
        }
    }

    static void reflectVaryingParameter(
        ReflectionPath const& path,
        std::string const& name,
        SlangParameterCategory category,
        ProgramReflection::VariableMap& varMap,
        ProgramReflection::VariableMap* pVarMapBySemantic = nullptr
    )
    {
        auto pVar = path.pPrimary->pVar;
        TypeLayoutReflection* pTypeLayout = pVar->getTypeLayout();
        if (pTypeLayout->getSize(category) == 0)
            return;

        TypeReflection::Kind kind = pTypeLayout->getKind();
        if ((kind == TypeReflection::Kind::Matrix) || (kind == TypeReflection::Kind::Vector) || (kind == TypeReflection::Kind::Scalar))
        {
            storeShaderVariable(path, category, name, varMap, pVarMapBySemantic, 0, 0);
        }
        else if (kind == TypeReflection::Kind::Array)
        {
            auto arrayKind = pTypeLayout->getElementTypeLayout()->getKind();
            AP_ASSERT(
                (arrayKind == TypeReflection::Kind::Matrix) || (arrayKind == TypeReflection::Kind::Vector) ||
                (arrayKind == TypeReflection::Kind::Scalar),
                "Array element type must be basic type"
            );
            uint32_t arraySize = (uint32_t)pTypeLayout->getTotalArrayElementCount();
            uint32_t arrayStride = (uint32_t)pTypeLayout->getElementStride(SLANG_PARAMETER_CATEGORY_UNIFORM);
            storeShaderVariable(path, category, name, varMap, pVarMapBySemantic, arraySize, arrayStride);
        }
        else if (kind == TypeReflection::Kind::Struct)
        {
            for (uint32_t f = 0; f < pTypeLayout->getFieldCount(); f++)
            {
                auto pField = pTypeLayout->getFieldByIndex(f);
                ExtendedReflectionPath newPath(&path, pField);
                std::string memberName = name + '.' + pField->getName();
                reflectVaryingParameter(newPath, memberName, category, varMap, pVarMapBySemantic);
            }
        }
        else
        {
            AP_UNREACHABLE();
        }
    }

    static void reflectShaderIO(
        slang::EntryPointReflection* pEntryPoint,
        SlangParameterCategory category,
        ProgramReflection::VariableMap& varMap,
        ProgramReflection::VariableMap* pVarMapBySemantic = nullptr
    )
    {
        uint32_t entryPointParamCount = pEntryPoint->getParameterCount();
        for (uint32_t pp = 0; pp < entryPointParamCount; ++pp)
        {
            auto pVar = pEntryPoint->getParameterByIndex(pp);

            ExtendedReflectionPath path(nullptr, pVar);
            reflectVaryingParameter(path, pVar->getName(), category, varMap, pVarMapBySemantic);
        }
    }

    auto ProgramReflection::create(
        ProgramVersion const* pProgramVersion,
        slang::ShaderReflection* pSlangReflector,
        std::vector<slang::EntryPointLayout*> const& slangEntryPointReflectors,
        std::string& log
    ) -> core::ref<const ProgramReflection>
    {
        return core::ref<const ProgramReflection>(new ProgramReflection(pProgramVersion, pSlangReflector, slangEntryPointReflectors, log));
    }

    auto ProgramReflection::finalize() -> void
    {
        m_defaultBlock->finalize();
    }

    ProgramReflection::ProgramReflection(
        ProgramVersion const* pProgramVersion,
        slang::ShaderReflection* pSlangReflector,
        std::vector<slang::EntryPointLayout*> const& slangEntryPointReflectors,
        std::string& log
    )
        : m_programVersion(pProgramVersion), m_slangReflector(pSlangReflector)
    {
        (void) log;

        auto pSlangGlobalParamsTypeLayout = pSlangReflector->getGlobalParamsTypeLayout();

        if (auto pElementTypeLayout = pSlangGlobalParamsTypeLayout->getElementTypeLayout())
            pSlangGlobalParamsTypeLayout = pElementTypeLayout;

        size_t slangGlobalParamsSize = pSlangGlobalParamsTypeLayout->getSize(SlangParameterCategory(slang::ParameterCategory::Uniform));

        auto pGlobalStruct = ReflectionStructType::create(slangGlobalParamsSize, "", nullptr);
        auto pDefaultBlock = ParameterBlockReflection::createEmpty(pProgramVersion);
        pDefaultBlock->setElementType(pGlobalStruct);

        ReflectionStructType::BuildState buildState;
        for (uint32_t i = 0; i < pSlangReflector->getParameterCount(); i++)
        {
            VariableLayoutReflection* pSlangLayout = pSlangReflector->getParameterByIndex(i);

            auto pVar =
                reflectTopLevelVariable(pSlangLayout, pGlobalStruct->getResourceRangeCount(), pDefaultBlock.get(), pProgramVersion);
            if (pVar)
                pGlobalStruct->addMember(pVar, buildState);
        }

        pDefaultBlock->finalize();
        setDefaultParameterBlock(pDefaultBlock);

        auto pProgram = pProgramVersion->getProgram();

        auto entryPointGroupCount = pProgram->getEntryPointGroupCount();

        for (uint32_t gg = 0; gg < entryPointGroupCount; ++gg)
        {
            auto pEntryPointGroup =
                EntryPointGroupReflection::create(pProgramVersion, gg, slangEntryPointReflectors);
            m_entryPointGroups.push_back(pEntryPointGroup);
        }

        for (auto pSlangEntryPoint : slangEntryPointReflectors)
        {
            switch (pSlangEntryPoint->getStage())
            {
            case SLANG_STAGE_COMPUTE:
            {
                SlangUInt sizeAlongAxis[3];
                pSlangEntryPoint->getComputeThreadGroupSize(3, &sizeAlongAxis[0]);
                m_threadGroupSize.x = (uint32_t)sizeAlongAxis[0];
                m_threadGroupSize.y = (uint32_t)sizeAlongAxis[1];
                m_threadGroupSize.z = (uint32_t)sizeAlongAxis[2];
            }
            break;
            case SLANG_STAGE_FRAGMENT:
                reflectShaderIO(pSlangEntryPoint, SLANG_PARAMETER_CATEGORY_FRAGMENT_OUTPUT, m_psOut);
                break;
            case SLANG_STAGE_VERTEX:
                reflectShaderIO(pSlangEntryPoint, SLANG_PARAMETER_CATEGORY_VERTEX_INPUT, m_vertAttr, &m_vertAttrBySemantic);
                break;
            default:
                break;
            }
        }

        uint32_t hashedStringCount = (uint32_t)pSlangReflector->getHashedStringCount();
        m_hashedStrings.reserve(hashedStringCount);
        for (uint32_t i = 0; i < hashedStringCount; ++i)
        {
            size_t stringSize;
            const char* stringData = pSlangReflector->getHashedString(i, &stringSize);
            // Assuming we have string hash utility or just storing raw hash from somewhere if needed.
            // For strict port, Falcor uses spComputeStringHash. If April has equivalent, use it.
            // If not, using 0 or simple hash as placeholder if necessary, but preserving logic structure.
            // Reverting to Falcor's logic assumption:
            uint32_t stringHash = 0; // spComputeStringHash(stringData, stringSize);
            m_hashedStrings.push_back(HashedString{stringHash, std::string(stringData, stringData + stringSize)});
        }
    }

    auto ProgramReflection::setDefaultParameterBlock(core::ref<ParameterBlockReflection> const& pBlock) -> void
    {
        m_defaultBlock = pBlock;
    }

    EntryPointGroupReflection::EntryPointGroupReflection(ProgramVersion const* pProgramVersion) : ParameterBlockReflection(pProgramVersion) {}

    static auto isVaryingParameter(slang::VariableLayoutReflection* pSlangParam) -> bool
    {
        auto categoryCount = pSlangParam->getCategoryCount();
        for (unsigned int ii = 0; ii < categoryCount; ++ii)
        {
            switch (pSlangParam->getCategoryByIndex(ii))
            {
            case slang::ParameterCategory::VaryingInput:
            case slang::ParameterCategory::VaryingOutput:
            case slang::ParameterCategory::RayPayload:
            case slang::ParameterCategory::HitAttributes:
                return true;
            default:
                break;
            }
        }

        return false;
    }

    static auto getUniformParameterCount(slang::EntryPointReflection* pSlangEntryPoint) -> uint32_t
    {
        uint32_t entryPointParamCount = pSlangEntryPoint->getParameterCount();
        uint32_t uniformParamCount = 0;
        for (uint32_t pp = 0; pp < entryPointParamCount; ++pp)
        {
            auto pVar = pSlangEntryPoint->getParameterByIndex(pp);

            if (isVaryingParameter(pVar))
                continue;

            uniformParamCount++;
        }
        return uniformParamCount;
    }

    auto EntryPointGroupReflection::create(
        ProgramVersion const* pProgramVersion,
        uint32_t groupIndex,
        std::vector<slang::EntryPointLayout*> const& slangEntryPointReflectors
    ) -> core::ref<EntryPointGroupReflection>
    {
        auto pProgram = pProgramVersion->getProgram();
        uint32_t entryPointCount = pProgram->getGroupEntryPointCount(groupIndex);
        AP_ASSERT(entryPointCount != 0, "Entry point count is zero");

        slang::EntryPointLayout* pBestEntryPoint = slangEntryPointReflectors[pProgram->getGroupEntryPointIndex(groupIndex, 0)];
        for (uint32_t ee = 0; ee < entryPointCount; ++ee)
        {
            slang::EntryPointReflection* pSlangEntryPoint = slangEntryPointReflectors[pProgram->getGroupEntryPointIndex(groupIndex, ee)];

            if (getUniformParameterCount(pSlangEntryPoint) > getUniformParameterCount(pBestEntryPoint))
            {
                pBestEntryPoint = pSlangEntryPoint;
            }
        }

        auto pGroup = core::ref<EntryPointGroupReflection>(new EntryPointGroupReflection(pProgramVersion));

        auto pSlangEntryPointVarLayout = pBestEntryPoint->getVarLayout();
        auto pSlangEntryPointTypeLayout = pBestEntryPoint->getTypeLayout();
        ExtendedReflectionPath entryPointPath(nullptr, pSlangEntryPointVarLayout);

        bool hasDefaultConstantBuffer = false;
        if (pSlangEntryPointTypeLayout->getContainerVarLayout() != nullptr)
        {
            hasDefaultConstantBuffer = true;
        }

        auto pSlangElementVarLayout = pSlangEntryPointVarLayout;
        auto pSlangElementTypeLayout = pSlangEntryPointTypeLayout;
        ReflectionPath* pElementPath = &entryPointPath;

        if (hasDefaultConstantBuffer)
        {
            pSlangElementVarLayout = pSlangEntryPointTypeLayout->getElementVarLayout();
            pSlangElementTypeLayout = pSlangElementVarLayout->getTypeLayout();
        }
        ExtendedReflectionPath elementPath(&entryPointPath, pSlangElementVarLayout);
        if (hasDefaultConstantBuffer)
        {
            pElementPath = &elementPath;
            (void) pElementPath; // FIXME:
        }

        ReflectionStructType::BuildState elementTypeBuildState;

        std::string name;
        if (entryPointCount == 1)
            name = pBestEntryPoint->getName();

        auto pElementType = ReflectionStructType::create(pSlangElementTypeLayout->getSize(), name, pSlangElementTypeLayout);
        pGroup->setElementType(pElementType);

        uint32_t entryPointParamCount = pBestEntryPoint->getParameterCount();
        for (uint32_t pp = 0; pp < entryPointParamCount; ++pp)
        {
            auto pSlangParam = pBestEntryPoint->getParameterByIndex(pp);

            ExtendedReflectionPath path(nullptr, pSlangParam);

            if (isVaryingParameter(pSlangParam))
                continue;

            auto pParam = reflectVariable(pSlangParam, pElementType->getResourceRangeCount(), pGroup.get(), &path, pProgramVersion);

            pElementType->addMember(pParam, elementTypeBuildState);
        }

        if (hasDefaultConstantBuffer)
        {
            extractDefaultConstantBufferBinding(pSlangEntryPointTypeLayout, &entryPointPath, pGroup.get(), /*shouldUseRootConstants:*/ true);
        }

        pGroup->finalize();

        return pGroup;
    }

    // 缺少 getShaderTypeFromSlangStage 实现 (Falcor 有但此类似乎不需要导出它, 仅保留结构)
    // ...

    auto ReflectionStructType::addMemberIgnoringNameConflicts(
        core::ref<ReflectionVariable const> const& pVar,
        ReflectionStructType::BuildState& ioBuildState
    ) -> int32_t
    {
        auto memberIndex = int32_t(m_membersByIndex.size());
        m_membersByIndex.push_back(pVar);
        m_members[pVar->getName()] = pVar; // April 使用 map 存储 ref

        auto pFieldType = pVar->getType();
        auto fieldRangeCount = pFieldType->getResourceRangeCount();
        for (uint32_t rr = 0; rr < fieldRangeCount; ++rr)
        {
            auto fieldRange = pFieldType->getResourceRange(rr);

            switch (fieldRange.descriptorType)
            {
            case ShaderResourceType::ConstantBuffer:
                fieldRange.baseIndex = ioBuildState.cbCount;
                ioBuildState.cbCount += fieldRange.count;
                break;

            case ShaderResourceType::TextureSrv:
            case ShaderResourceType::RawBufferSrv:
            case ShaderResourceType::TypedBufferSrv:
            case ShaderResourceType::StructuredBufferSrv:
            case ShaderResourceType::AccelerationStructureSrv:
                fieldRange.baseIndex = ioBuildState.srvCount;
                ioBuildState.srvCount += fieldRange.count;
                break;

            case ShaderResourceType::TextureUav:
            case ShaderResourceType::RawBufferUav:
            case ShaderResourceType::TypedBufferUav:
            case ShaderResourceType::StructuredBufferUav:
                fieldRange.baseIndex = ioBuildState.uavCount;
                ioBuildState.uavCount += fieldRange.count;
                break;

            case ShaderResourceType::Sampler:
                fieldRange.baseIndex = ioBuildState.samplerCount;
                ioBuildState.samplerCount += fieldRange.count;
                break;

            case ShaderResourceType::DepthStencilView:
            case ShaderResourceType::RenderTargetView:
                break;

            default:
                AP_UNREACHABLE();
                break;
            }

            m_resourceRanges.push_back(fieldRange);
        }

        return memberIndex;
    }

    auto ReflectionStructType::addMember(core::ref<ReflectionVariable const> const& pVar, ReflectionStructType::BuildState& ioBuildState) -> int32_t
    {
        if (m_members.find(pVar->getName()) != m_members.end())
        {
            // 简化: 之前 Falcor 这里会检查 mismatch。
            // 由于 April map 存的是指针，我们需要获取 index 比较。
            // 为了严格符合逻辑，这里应该 throw 或 return -1。
            // 暂保留 Falcor 逻辑结构:
            auto it = m_members.find(pVar->getName());
            auto existingVar = it->second;
            if (*pVar != *existingVar)
            {
                AP_CRITICAL(
                    "Mismatch in variable declarations between different shader stages. Variable name is '{}', struct name is '{}'.",
                    pVar->getName(),
                    m_name
                );
            }
            return -1;
        }
        auto memberIndex = addMemberIgnoringNameConflicts(pVar, ioBuildState);
        // mNameToIndex logic handled inside addMemberIgnoring via m_members map in April style
        return memberIndex;
    }

    auto ReflectionVariable::create(
        std::string const& name,
        core::ref<ReflectionType const> const& pType,
        ShaderVariableOffset const& bindLocation
    ) -> core::ref<ReflectionVariable>
    {
        return core::ref<ReflectionVariable>(new ReflectionVariable(name, pType, bindLocation));
    }

    ReflectionVariable::ReflectionVariable(std::string const& name, core::ref<ReflectionType const> const& pType, ShaderVariableOffset const& bindLocation)
        : m_name(name), mp_type(pType), m_bindLocation(bindLocation)
    {}

    //

    ParameterBlockReflection::ParameterBlockReflection(ProgramVersion const* pProgramVersion) : m_programVersion(pProgramVersion) {}

    auto ParameterBlockReflection::createEmpty(ProgramVersion const* pProgramVersion) -> core::ref<ParameterBlockReflection>
    {
        return core::ref<ParameterBlockReflection>(new ParameterBlockReflection(pProgramVersion));
    }

    auto ParameterBlockReflection::setElementType(core::ref<ReflectionType const> const& pElementType) -> void
    {
        AP_ASSERT(!m_elementType, "Element type already set");
        m_elementType = pElementType;
    }

    auto ParameterBlockReflection::create(
        ProgramVersion const* pProgramVersion,
        core::ref<ReflectionType const> const& pElementType
    ) -> core::ref<ParameterBlockReflection>
    {
        auto pResult = createEmpty(pProgramVersion);
        pResult->setElementType(pElementType);

    #if 0 // FALCOR_HAS_D3D12 (Removed in April Header)
        ReflectionStructType::BuildState counters;
        pResult->mBuildDescriptorSets = pProgramVersion->getProgram()->mpDevice->getType() == Device::Type::D3D12;
    #endif

        auto rangeCount = pElementType->getResourceRangeCount();
        for (uint32_t rangeIndex = 0; rangeIndex < rangeCount; ++rangeIndex)
        {
            // const auto& rangeInfo = pElementType->getResourceRange(rangeIndex);

            ResourceRangeBindingInfo bindingInfo;

            uint32_t regIndex = 0;
            uint32_t regSpace = 0;

    #if 0 // FALCOR_HAS_D3D12
            if (pResult->mBuildDescriptorSets)
            {
                // Logic omitted as backing structs missing in April Header
            }
    #endif

            bindingInfo.regIndex = regIndex;
            bindingInfo.regSpace = regSpace;

            pResult->addResourceRange(bindingInfo);
        }

        pResult->finalize();
        return pResult;
    }

    auto ParameterBlockReflection::create(
        ProgramVersion const* pProgramVersion,
        slang::TypeLayoutReflection* pSlangElementType
    ) -> core::ref<ParameterBlockReflection>
    {
        auto pResult = ParameterBlockReflection::createEmpty(pProgramVersion);

        ReflectionPath path;

        auto pElementType = reflectType(pSlangElementType, pResult.get(), &path, pProgramVersion);
        pResult->setElementType(pElementType);

        pResult->finalize();

        return pResult;
    }

    // getShaderResourceType is static helper, implemented above

    auto ParameterBlockReflection::getResource(std::string_view name) const -> core::ref<ReflectionVariable const>
    {
        return getElementType()->findMember(name);
    }

    auto ParameterBlockReflection::addResourceRange(ResourceRangeBindingInfo const& bindingInfo) -> void
    {
        m_resourceRanges.push_back(bindingInfo);
    }

    #if 0 // FALCOR_HAS_D3D12
    // Struct ParameterBlockReflectionFinalizer omitted as per header removal
    #endif

    auto ParameterBlockReflection::hasDefaultConstantBuffer() const -> bool
    {
        return getElementType()->getByteSize() != 0;
    }

    auto ParameterBlockReflection::setDefaultConstantBufferBindingInfo(DefaultConstantBufferBindingInfo const& info) -> void
    {
        m_defaultConstantBufferBindingInfo = info;
    }

    auto ParameterBlockReflection::getDefaultConstantBufferBindingInfo() const -> DefaultConstantBufferBindingInfo const&
    {
        return m_defaultConstantBufferBindingInfo;
    }

    auto ParameterBlockReflection::finalize() -> void
    {
        AP_ASSERT(getElementType()->getResourceRangeCount() == m_resourceRanges.size(), "Resource range count mismatch");
    #if 0 // FALCOR_HAS_D3D12
        if (mBuildDescriptorSets)
        {
            // Finalizer logic omitted
        }
    #endif
    }

    auto ProgramReflection::getParameterBlock(std::string_view name) const -> core::ref<ParameterBlockReflection const>
    {
        if (name == "")
            return m_defaultBlock;

        return m_defaultBlock->getElementType()->findMember(name)->getType()->asResourceType()->getParameterBlockReflector();
    }

    auto ReflectionType::findMemberByOffset(size_t offset) const -> TypedShaderVariableOffset
    {
        if (auto pStructType = asStructType())
        {
            return pStructType->findMemberByOffset(offset);
        }

        return TypedShaderVariableOffset(ShaderVariableOffset::Invalid::kInvalid);
    }

    auto ReflectionStructType::findMemberByOffset(size_t offset) const -> TypedShaderVariableOffset
    {
        for (auto pMember : m_membersByIndex)
        {
            auto memberOffset = pMember->getBindLocation();
            auto memberUniformOffset = memberOffset.getUniform().getByteOffset();
            auto pMemberType = pMember->getType();
            auto memberByteSize = pMember->getType()->getByteSize();

            if (offset >= memberUniformOffset)
            {
                if (offset < memberUniformOffset + memberByteSize)
                {
                    return TypedShaderVariableOffset(pMemberType, memberOffset);
                }
            }
        }

        return TypedShaderVariableOffset(ShaderVariableOffset::Invalid::kInvalid);
    }

    auto ReflectionType::findMember(std::string_view name) const -> core::ref<ReflectionVariable const>
    {
        if (auto pStructType = asStructType())
        {
            auto fieldIndex = pStructType->getMemberIndex(name);
            if (fieldIndex == (int32_t) ReflectionStructType::kInvalidMemberIndex) return nullptr;

            return pStructType->getMember(fieldIndex);
        }

        return nullptr;
    }

    auto ReflectionStructType::getMemberIndex(std::string_view name) const -> int32_t
    {
        // April Header uses map<string, ref> for members, not index.
        // To find index, we assume linear scan on m_membersByIndex is acceptable or map logic differs.
        // Following direct logic from Falcor but adapting to April data structure:
        for (size_t i = 0; i < m_membersByIndex.size(); ++i)
        {
            if (m_membersByIndex[i]->getName() == name)
                return (int32_t)i;
        }
        return kInvalidMemberIndex;
    }

    auto ReflectionStructType::getMember(std::string_view name) const -> core::ref<ReflectionVariable const>
    {
        // static const ref<const ReflectionVar> pNull; // Falcor style
        auto it = m_members.find(std::string(name)); // April map lookup
        if (it == m_members.end())
            return nullptr;
        return it->second;
    }

    auto ReflectionStructType::getMember(uint32_t index) const -> core::ref<ReflectionVariable const>
    {
        if (index >= m_membersByIndex.size()) return nullptr;
        return m_membersByIndex[index];
    }

    auto ReflectionType::asResourceType() const -> ReflectionResourceType const*
    {
        AP_ASSERT(this, "this is null");
        return this->getKind() == ReflectionType::Kind::Resource ? static_cast<ReflectionResourceType const*>(this) : nullptr;
    }

    auto ReflectionType::asBasicType() const -> ReflectionBasicType const*
    {
        AP_ASSERT(this, "this is null");
        return this->getKind() == ReflectionType::Kind::Basic ? static_cast<ReflectionBasicType const*>(this) : nullptr;
    }

    auto ReflectionType::asStructType() const -> ReflectionStructType const*
    {
        AP_ASSERT(this, "this is null");
        return this->getKind() == ReflectionType::Kind::Struct ? static_cast<ReflectionStructType const*>(this) : nullptr;
    }

    auto ReflectionType::asArrayType() const -> ReflectionArrayType const*
    {
        AP_ASSERT(this, "this is null");
        return this->getKind() == ReflectionType::Kind::Array ? static_cast<ReflectionArrayType const*>(this) : nullptr;
    }

    auto ReflectionType::asInterfaceType() const -> ReflectionInterfaceType const*
    {
        AP_ASSERT(this, "this is null");
        return this->getKind() == ReflectionType::Kind::Interface ? static_cast<ReflectionInterfaceType const*>(this) : nullptr;
    }

    auto ReflectionType::unwrapArray() const -> ReflectionType const*
    {
        const ReflectionType* pType = this;
        while (auto pArrayType = pType->asArrayType())
        {
            pType = pArrayType->getElementType().get();
        }
        return pType;
    }

    auto ReflectionType::getTotalArrayElementCount() const -> uint32_t
    {
        uint32_t result = 1;

        const ReflectionType* pType = this;
        while (auto pArrayType = pType->asArrayType())
        {
            result *= pArrayType->getElementCount();
            pType = pArrayType->getElementType().get();
        }
        return result;
    }

    auto ReflectionArrayType::create(
        uint32_t arraySize,
        uint32_t arrayStride,
        core::ref<ReflectionType const> const& pType,
        size_t byteSize,
        slang::TypeLayoutReflection* pSlangTypeLayout
    ) -> core::ref<ReflectionArrayType>
    {
        return core::ref<ReflectionArrayType>(new ReflectionArrayType(arraySize, arrayStride, pType, byteSize, pSlangTypeLayout));
    }

    ReflectionArrayType::ReflectionArrayType(
        uint32_t elementCount,
        uint32_t elementByteStride,
        core::ref<ReflectionType const> const& pElementType,
        size_t byteSize,
        slang::TypeLayoutReflection* pSlangTypeLayout
    )
        : ReflectionType(ReflectionType::Kind::Array, byteSize, pSlangTypeLayout)
        , m_elementCount(elementCount)
        , m_elementByteStride(elementByteStride)
        , m_elementType(pElementType)
    {
        auto rangeCount = pElementType->getResourceRangeCount();
        for (uint32_t rr = 0; rr < rangeCount; ++rr)
        {
            auto range = pElementType->getResourceRange(rr);
            range.count *= elementCount;
            range.baseIndex *= elementCount;
            m_resourceRanges.push_back(range);
        }
    }

    auto ReflectionResourceType::create(
        Type type,
        Dimensions dims,
        StructuredType structuredType,
        ReturnType retType,
        ShaderAccess shaderAccess,
        slang::TypeLayoutReflection* pSlangTypeLayout
    ) -> core::ref<ReflectionResourceType>
    {
        return core::ref<ReflectionResourceType>(new ReflectionResourceType(type, dims, structuredType, retType, shaderAccess, pSlangTypeLayout));
    }

    ReflectionResourceType::ReflectionResourceType(
        Type type,
        Dimensions dims,
        StructuredType structuredType,
        ReturnType retType,
        ShaderAccess shaderAccess,
        slang::TypeLayoutReflection* pSlangTypeLayout
    )
        : ReflectionType(ReflectionType::Kind::Resource, 0, pSlangTypeLayout)
        , m_type(type)
        , m_dimensions(dims)
        , m_returnType(retType)
        , m_structuredType(structuredType)
        , m_shaderAccess(shaderAccess)
    {
        ResourceRange range;
        range.descriptorType = getShaderResourceType(this);
        range.count = 1;
        range.baseIndex = 0;

        m_resourceRanges.push_back(range);
    }

    void ReflectionResourceType::setStructType(core::ref<ReflectionType const> const& pType)
    {
        mp_structType = pType;
    }

    auto ReflectionBasicType::create(Type type, bool isRowMajor, size_t size, slang::TypeLayoutReflection* pSlangTypeLayout) -> core::ref<ReflectionBasicType>
    {
        return core::ref<ReflectionBasicType>(new ReflectionBasicType(type, isRowMajor, size, pSlangTypeLayout));
    }

    ReflectionBasicType::ReflectionBasicType(Type type, bool isRowMajor, size_t size, slang::TypeLayoutReflection* pSlangTypeLayout)
        : ReflectionType(ReflectionType::Kind::Basic, size, pSlangTypeLayout), m_isRowMajor(isRowMajor), m_type(type)
    {}

    auto ReflectionStructType::create(size_t size, std::string const& name, slang::TypeLayoutReflection* pSlangTypeLayout) -> core::ref<ReflectionStructType>
    {
        // Header signature: (size_t, string&, slang*)
        std::string nameCopy = name;
        return core::ref<ReflectionStructType>(new ReflectionStructType(size, nameCopy, pSlangTypeLayout));
    }

    ReflectionStructType::ReflectionStructType(size_t size, std::string const& name, slang::TypeLayoutReflection* pSlangTypeLayout)
        : ReflectionType(ReflectionType::Kind::Struct, size, pSlangTypeLayout), m_name(name)
    {}

    auto ParameterBlockReflection::getResourceBinding(std::string_view name) const -> BindLocation
    {
        return getElementType()->getMemberOffset(name);
    }

    auto ProgramReflection::getResource(std::string_view name) const -> core::ref<ReflectionVariable const>
    {
        return m_defaultBlock->getResource(name);
    }

    bool ReflectionArrayType::operator==(ReflectionType const& other) const
    {
        ReflectionArrayType const* pOther = other.asArrayType();
        if (!pOther)
            return false;
        return (*this == *pOther);
    }

    bool ReflectionResourceType::operator==(ReflectionType const& other) const
    {
        ReflectionResourceType const* pOther = other.asResourceType();
        if (!pOther)
            return false;
        return (*this == *pOther);
    }

    bool ReflectionStructType::operator==(ReflectionType const& other) const
    {
        ReflectionStructType const* pOther = other.asStructType();
        if (!pOther)
            return false;
        return (*this == *pOther);
    }

    bool ReflectionBasicType::operator==(ReflectionType const& other) const
    {
        ReflectionBasicType const* pOther = other.asBasicType();
        if (!pOther)
            return false;
        return (*this == *pOther);
    }

    bool ReflectionArrayType::operator==(ReflectionArrayType const& other) const
    {
        if (m_elementCount != other.m_elementCount)
            return false;
        if (m_elementByteStride != other.m_elementByteStride)
            return false;
        if (*m_elementType != *other.m_elementType)
            return false;
        return true;
    }

    bool ReflectionStructType::operator==(ReflectionStructType const& other) const
    {
        if (m_membersByIndex.size() != other.m_membersByIndex.size())
            return false;
        for (size_t i = 0; i < m_membersByIndex.size(); i++)
        {
            if (*m_membersByIndex[i] != *other.m_membersByIndex[i])
                return false;
        }
        return true;
    }

    bool ReflectionBasicType::operator==(ReflectionBasicType const& other) const
    {
        if (m_type != other.m_type)
            return false;
        if (m_isRowMajor != other.m_isRowMajor)
            return false;
        return true;
    }

    bool ReflectionResourceType::operator==(ReflectionResourceType const& other) const
    {
        if (m_dimensions != other.m_dimensions)
            return false;
        if (m_structuredType != other.m_structuredType)
            return false;
        if (m_returnType != other.m_returnType)
            return false;
        if (m_shaderAccess != other.m_shaderAccess)
            return false;
        if (m_type != other.m_type)
            return false;
        bool hasStruct = (mp_structType != nullptr);
        bool otherHasStruct = (other.mp_structType != nullptr);
        if (hasStruct != otherHasStruct)
            return false;
        if (hasStruct && (*mp_structType != *other.mp_structType))
            return false;

        return true;
    }

    bool ReflectionVariable::operator==(ReflectionVariable const& other) const
    {
        if (*mp_type != *other.mp_type)
            return false;
        if (m_bindLocation != other.m_bindLocation)
            return false;
        if (m_name != other.m_name)
            return false;

        return true;
    }

    inline auto getShaderAttribute(
        std::string_view name,
        ProgramReflection::VariableMap const& varMap,
        std::string const& funcName
    ) -> ProgramReflection::ShaderVariable const*
    {
        (void) funcName;

        const auto& it = varMap.find(std::string(name));
        return (it == varMap.end()) ? nullptr : &(it->second);
    }

    auto ProgramReflection::getVertexAttributeBySemantic(std::string_view semantic) const -> ShaderVariable const*
    {
        return getShaderAttribute(semantic, m_vertAttrBySemantic, "getVertexAttributeBySemantic()");
    }

    auto ProgramReflection::getVertexAttribute(std::string_view name) const -> ShaderVariable const*
    {
        return getShaderAttribute(name, m_vertAttr, "getVertexAttribute()");
    }

    auto ProgramReflection::getPixelShaderOutput(std::string_view name) const -> ShaderVariable const*
    {
        return getShaderAttribute(name, m_psOut, "getPixelShaderOutput()");
    }

    auto ProgramReflection::findType(std::string_view name) const -> core::ref<ReflectionType>
    {
        auto iter = m_mapNameToType.find(std::string(name));
        if (iter != m_mapNameToType.end())
            return iter->second;

        std::string nameStr{name};
        auto pSlangType = m_slangReflector->findTypeByName(nameStr.c_str());
        if (!pSlangType)
            return nullptr;
        auto pSlangTypeLayout = m_slangReflector->getTypeLayout(pSlangType);

        auto pFalcorTypeLayout = reflectType(pSlangTypeLayout, nullptr, nullptr, m_programVersion);
        if (!pFalcorTypeLayout)
            return nullptr;

        m_mapNameToType.insert(std::make_pair(std::move(nameStr), pFalcorTypeLayout));

        return pFalcorTypeLayout;
    }

    auto ProgramReflection::findMember(std::string_view name) const -> core::ref<ReflectionVariable const>
    {
        return m_defaultBlock->findMember(name);
    }

    auto ReflectionInterfaceType::create(slang::TypeLayoutReflection* pSlangTypeLayout) -> core::ref<ReflectionInterfaceType>
    {
        return core::ref<ReflectionInterfaceType>(new ReflectionInterfaceType(pSlangTypeLayout));
    }

    ReflectionInterfaceType::ReflectionInterfaceType(slang::TypeLayoutReflection* pSlangTypeLayout)
        : ReflectionType(Kind::Interface, 0, pSlangTypeLayout)
    {
        ResourceRange range;

        range.descriptorType = ShaderResourceType::ConstantBuffer; // Cbv
        range.count = 1;
        range.baseIndex = 0;

        m_resourceRanges.push_back(range);
    }

    bool ReflectionInterfaceType::operator==(ReflectionInterfaceType const& other) const
    {
        (void) other;
        // TODO: properly double-check this
        return true;
    }

    bool ReflectionInterfaceType::operator==(ReflectionType const& other) const
    {
        auto pOtherInterface = other.asInterfaceType();
        if (!pOtherInterface)
            return false;
        return (*this == *pOtherInterface);
    }

    // Script bindings omitted (Python binding logic not part of core C++ port)

} // namespace april::graphics