// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "rhi/types.hpp"

#include <core/foundation/object.hpp>
#include <core/math/type.hpp>
#include <core/tools/enum.hpp>
#include <slang.h>
#include <slang-com-ptr.h>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace april::graphics
{
    class ProgramVersion;
    class ReflectionVariable;
    class ReflectionType;
    class ReflectionResourceType;
    class ReflectionBasicType;
    class ReflectionStructType;
    class ReflectionArrayType;
    class ReflectionInterfaceType;
    class ParameterBlockReflection;

    /**
     * Represents the offset of a uniform shader variable relative to its enclosing type/buffer/block.
     *
     * A `UniformShaderVarOffset` is a simple wrapper around a byte offset for a uniform shader variable.
     * It is used to make API signatures less ambiguous (e.g., about whether an integer represents an
     * index, an offset, a count, etc.
     *
     * A `UniformShaderVarOffset` can also encode an invalid offset (represented as an all-ones bit pattern),
     * to indicate that a particular uniform variable is not present.
     *
     * A `UniformShaderVarOffset` can be obtained from a reflection type or `ParameterBlock` using the
     * `[]` subscript operator:
     *
     * UniformShaderVarOffset aOffset = pSomeType["a"]; // get offset of field `a` inside `pSomeType`
     * UniformShaderVarOffset bOffset = pBlock["b"]; // get offset of parameter `b` inside parameter block
     */
    struct UniformShaderVariableOffset
    {
        using ByteOffset = uint32_t;
        enum Zero { kZero = 0 };
        enum Invalid { kInvalid = -1 };

        UniformShaderVariableOffset(Invalid = kInvalid) : m_byteOffset(ByteOffset(-1)) {}
        UniformShaderVariableOffset(Zero) : m_byteOffset(0) {}
        explicit UniformShaderVariableOffset(size_t offset) : m_byteOffset(ByteOffset(offset)) {}

        auto getByteOffset() const -> ByteOffset { return m_byteOffset; }
        auto isValid() const -> bool { return m_byteOffset != ByteOffset(-1); }

        auto operator==(UniformShaderVariableOffset const& other) const -> bool { return m_byteOffset == other.m_byteOffset; }
        auto operator!=(UniformShaderVariableOffset const& other) const -> bool { return m_byteOffset != other.m_byteOffset; }
        auto operator==(Invalid) const -> bool { return !isValid(); }
        auto operator!=(Invalid) const -> bool { return isValid(); }

        auto operator+(size_t offset) const -> UniformShaderVariableOffset
        {
            if (!isValid()) return kInvalid;

            return UniformShaderVariableOffset(m_byteOffset + offset);
        }

        auto operator+(UniformShaderVariableOffset other) const -> UniformShaderVariableOffset
        {
            if (!isValid() || !other.isValid()) return kInvalid;

            return UniformShaderVariableOffset(m_byteOffset + other.m_byteOffset);
        }

    private:
        ByteOffset m_byteOffset{ByteOffset(-1)};
    };

    /**
     * Represents the offset of a resource-type shader variable relative to its enclosing type/buffer/block.
     *
     * A `ResourceShaderVarOffset` records the index of a descriptor range and an array index within that range.
     *
     * A `ResourceShaderVarOffset` can also encode an invalid offset (represented as an all-ones bit pattern
     * for both the range and array indices), to indicate that a particular resource variable is not present.
     *
     * A `ResourceShaderVarOffset` can be obtained from a reflection type or `ParameterBlock` using the
     * `[]` subscript operator:
     *
     * ResourceShaderVarOffset texOffset = pSomeType["tex"]; // get offset of texture `tex` inside `pSomeType`
     * ResourceShaderVarOffset sampOffset = pBlock["samp"]; // get offset of sampler `samp` inside block
     *
     * Please note that the concepts of resource "ranges" are largely an implementation detail of
     * the `ParameterBlock` type, and most user code should not attempt to explicitly work with
     * or reason about resource ranges. In particular, there is *no* correspondence between resource
     * range indices and the `register`s or `binding`s assigned to shader parameters.
     */
    struct ResourceShaderVariableOffset
    {
        using RangeIndex = uint32_t;
        using ArrayIndex = uint32_t;
        enum Zero { kZero = 0 };
        enum Invalid { kInvalid = -1 };

        ResourceShaderVariableOffset(Invalid = kInvalid) : m_rangeIndex(RangeIndex(-1)), m_arrayIndex(ArrayIndex(-1)) {}
        ResourceShaderVariableOffset(Zero) : m_rangeIndex(0), m_arrayIndex(0) {}
        ResourceShaderVariableOffset(RangeIndex rangeIdx, ArrayIndex arrayIdx) : m_rangeIndex(rangeIdx), m_arrayIndex(arrayIdx) {}
        explicit ResourceShaderVariableOffset(RangeIndex rangeIdx) : m_rangeIndex(rangeIdx), m_arrayIndex(0) {}

        auto isValid() const -> bool { return m_rangeIndex != RangeIndex(-1); }
        auto getRangeIndex() const -> RangeIndex { return m_rangeIndex; }
        auto getArrayIndex() const -> ArrayIndex { return m_arrayIndex; }

        auto operator==(ResourceShaderVariableOffset const& other) const -> bool
        {
            return m_rangeIndex == other.m_rangeIndex && m_arrayIndex == other.m_arrayIndex;
        }

        auto operator!=(ResourceShaderVariableOffset const& other) const -> bool { return !(*this == other); }

        auto operator+(ResourceShaderVariableOffset const& other) const -> ResourceShaderVariableOffset
        {
            if (!isValid() || !other.isValid()) return kInvalid;

            return ResourceShaderVariableOffset(m_rangeIndex + other.m_rangeIndex, m_arrayIndex + other.m_arrayIndex);
        }

    private:
        RangeIndex m_rangeIndex{(uint32_t) -1};
        ArrayIndex m_arrayIndex{(uint32_t) -1};
    };

    struct ShaderVariableOffset
    {
        enum Zero { kZero = 0 };
        enum Invalid { kInvalid = -1 };
        using ByteOffset = UniformShaderVariableOffset::ByteOffset;
        using RangeIndex = ResourceShaderVariableOffset::RangeIndex;
        using ArrayIndex = ResourceShaderVariableOffset::ArrayIndex;

        ShaderVariableOffset(Invalid = kInvalid) : m_uniform(UniformShaderVariableOffset::kInvalid), m_resource(ResourceShaderVariableOffset::kInvalid) {}
        ShaderVariableOffset(Zero) : m_uniform(UniformShaderVariableOffset::kZero), m_resource(ResourceShaderVariableOffset::kZero) {}
        ShaderVariableOffset(UniformShaderVariableOffset unif, ResourceShaderVariableOffset res) : m_uniform(unif), m_resource(res) {}

        auto isValid() const -> bool { return m_uniform.isValid(); }
        auto getUniform() const -> UniformShaderVariableOffset { return m_uniform; }
        auto getResource() const -> ResourceShaderVariableOffset { return m_resource; }
        auto getByteOffset() const -> ByteOffset { return m_uniform.getByteOffset(); }
        auto getRangeIndex() const -> RangeIndex { return m_resource.getRangeIndex(); }
        auto getResourceRangeIndex() const -> RangeIndex { return m_resource.getRangeIndex(); }
        auto getResourceArrayIndex() const -> RangeIndex { return m_resource.getArrayIndex(); }

        operator UniformShaderVariableOffset() const { return m_uniform; }
        operator ResourceShaderVariableOffset() const { return m_resource; }

        auto operator==(ShaderVariableOffset const& other) const -> bool { return m_uniform == other.m_uniform && m_resource == other.m_resource; }
        auto operator!=(ShaderVariableOffset const& other) const -> bool { return !(*this == other); }

        auto operator+(ShaderVariableOffset const& other) const -> ShaderVariableOffset
        {
            if (!isValid() || !other.isValid()) return kInvalid;

            return ShaderVariableOffset(m_uniform + other.m_uniform, m_resource + other.m_resource);
        }

    protected:
        UniformShaderVariableOffset m_uniform{};
        ResourceShaderVariableOffset m_resource{};
    };

    /**
     * Represents the type of a shader variable and its offset relative to its enclosing type/buffer/block.
     *
     * A `TypedShaderVarOffset` is just a `ShaderVarOffset` plus a `ReflectionType` for
     * the variable at the given offset.
     *
     * A `TypedShaderVarOffset` can also encode an invalid offset, to indicate that a particular
     * shader variable is not present.
     *
     * A `TypedShaderVarOffset` can be obtained from a reflection type or `ParameterBlock` using the
     * `[]` subscript operator:
     *
     * TypedShaderVarOffset lightOffset = pSomeType["light"]; // get type and offset of texture `light` inside `pSomeType`
     * TypedShaderVarOffset materialOffset = pBlock["material"]; // get type and offset of sampler `material` inside block
     *
     * In addition, a `TypedShaderVarOffset` can be used to look up offsets for
     * sub-fields/-elements of shader variables with structure or array types:
     *
     * UniformShaderVarOffset lightPosOffset = lightOffset["position"];
     * ResourceShaderVarOffset diffuseMapOffset = materialOffset["diffuseMap"];
     *
     * Such offsets are always relative to the root type or block where lookup started.
     * For example, in the above code `lightPosOffset` would be the offset of the
     * field `light.position` relative to the enclosing type `pSomeType` and *not*
     * the offset of the `position` field relative to the immediately enclosing `light` field.
     *
     * Because `TypedShaderVarOffset` inherits from `ShaderVarOffset` it can be used
     * in all the same places, and also implicitly converts to both
     * `UniformShaderVarOffset` and `ResourceShaderVarOffset`.
     *
     * This struct has a non-owning pointer to the type information.
     * The caller is responsible for ensuring that the type information remains valid,
     * which is typically owned by the `ParameterBlockReflection` object.
     */
    struct TypedShaderVariableOffset : ShaderVariableOffset
    {
        TypedShaderVariableOffset(Invalid = kInvalid) {}
        TypedShaderVariableOffset(ReflectionType const* pType, ShaderVariableOffset offset);

        auto getType() const -> ReflectionType const* { return m_type; }
        auto isValid() const -> bool { return m_type != nullptr; }

        auto operator[](std::string_view name) const -> TypedShaderVariableOffset;
        auto operator[](size_t index) const -> TypedShaderVariableOffset;

    private:
        ReflectionType const* m_type{nullptr};
    };

    class ReflectionType : public core::Object
    {
        APRIL_OBJECT(ReflectionType)
    public:
        enum class Kind
        {
            Array,
            Struct,
            Basic,
            Resource,
            Interface,
        };
        AP_ENUM_INFO(
            Kind,
            {
                {Kind::Array, "Array"},
                {Kind::Struct, "Struct"},
                {Kind::Basic, "Basic"},
                {Kind::Resource, "Resource"},
                {Kind::Interface, "Interface"},
            }
        );

        /**
         * A range of resources contained (directly or indirectly) in this type.
         *
         * Different types will contain different numbers of resources, and those
         * resources will always be grouped into contiguous "ranges" that must be
         * allocated together in descriptor sets to allow them to be indexed.
         *
         * Some examples:
         *
         * * A basic type like `float2` has zero resource ranges.
         *
         * * A resource type like `Texture2D` will have one resource range,
         * with a corresponding descriptor type and an array count of one.
         *
         * * An array type like `float2[3]` or `Texture2D[4]` will have
         * the same number of ranges as its element type, but the count
         * of each range will be multiplied by the array element count.
         *
         * * A structure type like `struct { Texture2D a; Texture2D b[3]; }`
         * will concatenate the resource ranges from its fields, in order.
         *
         * The `ResourceRange` type is mostly an implementation detail
         * of `ReflectionType` that supports `ParameterBlock` and users
         * should probably not rely on this information.
         */
        struct ResourceRange
        {
            ShaderResourceType descriptorType{ShaderResourceType::Unknown};
            uint32_t count{0};
            uint32_t baseIndex{0};
        };

        virtual ~ReflectionType() = default;

        auto getKind() const -> Kind { return m_kind; }
        auto getByteSize() const -> size_t { return m_byteSize; }
        auto getSlangTypeLayout() const -> slang::TypeLayoutReflection* { return m_slangTypeLayout; }

        auto asResourceType() const -> ReflectionResourceType const*;
        auto asBasicType() const -> ReflectionBasicType const*;
        auto asStructType() const -> ReflectionStructType const*;
        auto asArrayType() const -> ReflectionArrayType const*;
        auto asInterfaceType() const -> ReflectionInterfaceType const*;

        auto unwrapArray() const -> ReflectionType const*;
        auto getTotalArrayElementCount() const -> uint32_t;

        auto findMember(std::string_view name) const -> core::ref<ReflectionVariable const>;
        auto getMemberOffset(std::string_view name) const -> TypedShaderVariableOffset;
        auto findMemberByOffset(size_t byteOffset) const -> TypedShaderVariableOffset;
        auto getZeroOffset() const -> TypedShaderVariableOffset;
        auto getResourceRangeCount() const -> uint32_t { return static_cast<uint32_t>(m_resourceRanges.size()); }
        auto getResourceRange(uint32_t index) const -> ResourceRange const& { return m_resourceRanges[index]; }

        virtual auto operator==(ReflectionType const& other) const -> bool = 0;
        auto operator!=(ReflectionType const& other) const -> bool { return !(*this == other); }

    protected:
        ReflectionType(Kind kind, size_t byteSize, slang::TypeLayoutReflection* pSlangTypeLayout)
            : m_kind(kind), m_byteSize(byteSize), m_slangTypeLayout(pSlangTypeLayout) {}

        Kind m_kind{};
        size_t m_byteSize{0};
        std::vector<ResourceRange> m_resourceRanges{};
        slang::TypeLayoutReflection* m_slangTypeLayout{nullptr};
    };

    class ReflectionBasicType : public ReflectionType
    {
    public:
        enum class Type
        {
            Bool,
            Bool2,
            Bool3,
            Bool4,

            Uint8,
            Uint8_2,
            Uint8_3,
            Uint8_4,

            Uint16,
            Uint16_2,
            Uint16_3,
            Uint16_4,

            Uint,
            Uint2,
            Uint3,
            Uint4,

            Uint64,
            Uint64_2,
            Uint64_3,
            Uint64_4,

            Int8,
            Int8_2,
            Int8_3,
            Int8_4,

            Int16,
            Int16_2,
            Int16_3,
            Int16_4,

            Int,
            Int2,
            Int3,
            Int4,

            Int64,
            Int64_2,
            Int64_3,
            Int64_4,

            Float16,
            Float16_2,
            Float16_3,
            Float16_4,

            Float16_2x2,
            Float16_2x3,
            Float16_2x4,
            Float16_3x2,
            Float16_3x3,
            Float16_3x4,
            Float16_4x2,
            Float16_4x3,
            Float16_4x4,

            Float,
            Float2,
            Float3,
            Float4,

            Float2x2,
            Float2x3,
            Float2x4,
            Float3x2,
            Float3x3,
            Float3x4,
            Float4x2,
            Float4x3,
            Float4x4,

            Float64,
            Float64_2,
            Float64_3,
            Float64_4,

            Unknown = -1
        };
        AP_ENUM_INFO(
            Type,
            {
                {Type::Bool, "Bool"},
                {Type::Bool2, "Bool2"},
                {Type::Bool3, "Bool3"},
                {Type::Bool4, "Bool4"},
                {Type::Uint8, "Uint8"},
                {Type::Uint8_2, "Uint8_2"},
                {Type::Uint8_3, "Uint8_3"},
                {Type::Uint8_4, "Uint8_4"},
                {Type::Uint16, "Uint16"},
                {Type::Uint16_2, "Uint16_2"},
                {Type::Uint16_3, "Uint16_3"},
                {Type::Uint16_4, "Uint16_4"},
                {Type::Uint, "Uint"},
                {Type::Uint2, "Uint2"},
                {Type::Uint3, "Uint3"},
                {Type::Uint4, "Uint4"},
                {Type::Uint64, "Uint64"},
                {Type::Uint64_2, "Uint64_2"},
                {Type::Uint64_3, "Uint64_3"},
                {Type::Uint64_4, "Uint64_4"},
                {Type::Int8, "Int8"},
                {Type::Int8_2, "Int8_2"},
                {Type::Int8_3, "Int8_3"},
                {Type::Int8_4, "Int8_4"},
                {Type::Int16, "Int16"},
                {Type::Int16_2, "Int16_2"},
                {Type::Int16_3, "Int16_3"},
                {Type::Int16_4, "Int16_4"},
                {Type::Int, "Int"},
                {Type::Int2, "Int2"},
                {Type::Int3, "Int3"},
                {Type::Int4, "Int4"},
                {Type::Int64, "Int64"},
                {Type::Int64_2, "Int64_2"},
                {Type::Int64_3, "Int64_3"},
                {Type::Int64_4, "Int64_4"},
                {Type::Float16, "Float16"},
                {Type::Float16_2, "Float16_2"},
                {Type::Float16_3, "Float16_3"},
                {Type::Float16_4, "Float16_4"},
                {Type::Float16_2x2, "Float16_2x2"},
                {Type::Float16_2x3, "Float16_2x3"},
                {Type::Float16_2x4, "Float16_2x4"},
                {Type::Float16_3x2, "Float16_3x2"},
                {Type::Float16_3x3, "Float16_3x3"},
                {Type::Float16_3x4, "Float16_3x4"},
                {Type::Float16_4x2, "Float16_4x2"},
                {Type::Float16_4x3, "Float16_4x3"},
                {Type::Float16_4x4, "Float16_4x4"},
                {Type::Float, "Float"},
                {Type::Float2, "Float2"},
                {Type::Float3, "Float3"},
                {Type::Float4, "Float4"},
                {Type::Float2x2, "Float2x2"},
                {Type::Float2x3, "Float2x3"},
                {Type::Float2x4, "Float2x4"},
                {Type::Float3x2, "Float3x2"},
                {Type::Float3x3, "Float3x3"},
                {Type::Float3x4, "Float3x4"},
                {Type::Float4x2, "Float4x2"},
                {Type::Float4x3, "Float4x3"},
                {Type::Float4x4, "Float4x4"},
                {Type::Float64, "Float64"},
                {Type::Float64_2, "Float64_2"},
                {Type::Float64_3, "Float64_3"},
                {Type::Float64_4, "Float64_4"},
                {Type::Unknown, "Unknown"},
            }
        );

        // Use April equivalent for types if possible, else Slang types
        static auto create(Type type, bool isRowMajor, size_t size, slang::TypeLayoutReflection* slangTypeLayout) -> core::ref<ReflectionBasicType>;

        auto getType() const -> Type { return m_type; }
        auto isRowMajor() const { return m_isRowMajor; }

        auto operator==(ReflectionBasicType const& other) const -> bool;
        auto operator==(ReflectionType const& other) const -> bool override;

    private:
        ReflectionBasicType(Type type, bool isRowMajor, size_t size, slang::TypeLayoutReflection* pSlangTypeLayout);

        bool m_isRowMajor{false};
        Type m_type{Type::Unknown};
    };

    class ReflectionResourceType : public ReflectionType
    {
    public:
        /**
        * Describes how the shader will access the resource
        */
        enum class ShaderAccess
        {
            Undefined,
            Read,
            ReadWrite
        };
        AP_ENUM_INFO(
            ShaderAccess,
            {
                {ShaderAccess::Undefined, "Undefined"},
                {ShaderAccess::Read, "Read"},
                {ShaderAccess::ReadWrite, "ReadWrite"},
            }
        );

        /**
        * The expected return type
        */
        enum class ReturnType
        {
            Unknown,
            Float,
            Double,
            Int,
            Uint
        };
        AP_ENUM_INFO(
            ReturnType,
            {
                {ReturnType::Unknown, "Unknown"},
                {ReturnType::Float, "Float"},
                {ReturnType::Double, "Double"},
                {ReturnType::Int, "Int"},
                {ReturnType::Uint, "Uint"},
            }
        );

        /**
        * The resource dimension
        */
        enum class Dimensions
        {
            Unknown,
            Texture1D,
            Texture2D,
            Texture3D,
            TextureCube,
            Texture1DArray,
            Texture2DArray,
            Texture2DMS,
            Texture2DMSArray,
            TextureCubeArray,
            AccelerationStructure,
            Buffer,

            Count
        };
        AP_ENUM_INFO(
            Dimensions,
            {
                {Dimensions::Unknown, "Unknown"},
                {Dimensions::Texture1D, "Texture1D"},
                {Dimensions::Texture2D, "Texture2D"},
                {Dimensions::Texture3D, "Texture3D"},
                {Dimensions::TextureCube, "TextureCube"},
                {Dimensions::Texture1DArray, "Texture1DArray"},
                {Dimensions::Texture2DArray, "Texture2DArray"},
                {Dimensions::Texture2DMS, "Texture2DMS"},
                {Dimensions::Texture2DMSArray, "Texture2DMSArray"},
                {Dimensions::TextureCubeArray, "TextureCubeArray"},
                {Dimensions::AccelerationStructure, "AccelerationStructure"},
                {Dimensions::Buffer, "Buffer"},
            }
        );

        /**
        * For structured-buffers, describes the type of the buffer
        */
        enum class StructuredType
        {
            Invalid, ///< Not a structured buffer
            Default, ///< Regular structured buffer
            Counter, ///< RWStructuredBuffer with counter
            Append,  ///< AppendStructuredBuffer
            Consume  ///< ConsumeStructuredBuffer
        };
        AP_ENUM_INFO(
            StructuredType,
            {
                {StructuredType::Invalid, "Invalid"},
                {StructuredType::Default, "Default"},
                {StructuredType::Counter, "Counter"},
                {StructuredType::Append, "Append"},
                {StructuredType::Consume, "Consume"},
            }
        );

        /**
        * The type of the resource
        */
        enum class Type
        {
            Texture,
            StructuredBuffer,
            RawBuffer,
            TypedBuffer,
            Sampler,
            ConstantBuffer,
            AccelerationStructure,
        };
        AP_ENUM_INFO(
            Type,
            {
                {Type::Texture, "Texture"},
                {Type::StructuredBuffer, "StructuredBuffer"},
                {Type::RawBuffer, "RawBuffer"},
                {Type::TypedBuffer, "TypedBuffer"},
                {Type::Sampler, "Sampler"},
                {Type::ConstantBuffer, "ConstantBuffer"},
                {Type::AccelerationStructure, "AccelerationStructure"},
            }
        );

        static auto create(
            Type type,
            Dimensions dims,
            StructuredType structuredType,
            ReturnType retType,
            ShaderAccess shaderAccess,
            slang::TypeLayoutReflection* slangTypeLayout
        ) -> core::ref<ReflectionResourceType>;

        auto operator==(ReflectionResourceType const& other) const -> bool;
        auto operator==(ReflectionType const& other) const -> bool override;

        auto getType() const -> Type { return m_type; }
        auto getSize() const -> size_t { return mp_structType ? mp_structType->getByteSize() : 0; }
        auto getDimensions() const -> Dimensions { return m_dimensions; }
        auto getStructuredBufferType() const -> StructuredType { return m_structuredType; }
        auto getShaderAccess() const -> ShaderAccess { return m_shaderAccess; }
        auto getReturnType() const -> ReturnType { return m_returnType; }

        // Structure type for StructuredBuffer
        auto setStructType(core::ref<ReflectionType const> const& p_type) -> void;
        auto getStructType() const -> ReflectionType const* { return mp_structType.get(); }

        auto getParameterBlockReflector() const -> core::ref<const ParameterBlockReflection> const& { return mp_parameterBlockReflector; }
        auto setParameterBlockReflector(core::ref<const ParameterBlockReflection> const& p_reflector) -> void { mp_parameterBlockReflector = p_reflector; }

    protected:
        ReflectionResourceType(
            Type type,
            Dimensions dims,
            StructuredType structuredType,
            ReturnType retType,
            ShaderAccess shaderAccess,
            slang::TypeLayoutReflection* pSlangTypeLayout
        );

        Type m_type{Type::Texture};
        Dimensions m_dimensions{Dimensions::Unknown};
        ReturnType m_returnType{ReturnType::Unknown};
        StructuredType m_structuredType{StructuredType::Invalid};
        ShaderAccess m_shaderAccess{ShaderAccess::Read};
        core::ref<ReflectionType const> mp_structType{};
        core::ref<const ParameterBlockReflection> mp_parameterBlockReflector{};
    };

    class ReflectionStructType : public ReflectionType
    {
    public:
        static constexpr int32_t kInvalidMemberIndex{-1};

        static auto create(
            size_t byteSize,
            std::string const& name,
            slang::TypeLayoutReflection* slangTypeLayout
        ) -> core::ref<ReflectionStructType>;

        auto operator==(ReflectionStructType const& other) const -> bool;
        auto operator==(ReflectionType const& other) const -> bool override;

        auto getName() const -> std::string const& { return m_name; }
        auto getMemberCount() const -> uint32_t { return static_cast<uint32_t>(m_members.size()); }
        auto getMember(std::string_view name) const -> core::ref<ReflectionVariable const>;
        auto getMember(uint32_t index) const -> core::ref<ReflectionVariable const>;
        auto getMemberIndex(std::string_view name) const -> int32_t;
        auto findMemberByOffset(size_t offset) const -> TypedShaderVariableOffset;

        struct BuildState
        {
            uint32_t cbCount{};
            uint32_t srvCount{};
            uint32_t uavCount{};
            uint32_t samplerCount{};
        };

        auto addMember(core::ref<ReflectionVariable const> const& variable, BuildState& ioBuildState) -> int32_t;
        auto addMemberIgnoringNameConflicts(core::ref<ReflectionVariable const> const& variable, BuildState& ioBuildState) -> int32_t;

    protected:
        ReflectionStructType(size_t byteSize, std::string const& name, slang::TypeLayoutReflection* pSlangTypeLayout);

        std::string m_name{};
        std::vector<core::ref<ReflectionVariable const>> m_membersByIndex{};
        std::map<std::string, core::ref<ReflectionVariable const>, std::less<>> m_members{};
    };

    class ReflectionArrayType : public ReflectionType
    {
        APRIL_OBJECT(ReflectionArrayType)
    public:
        static auto create(
            uint32_t elementCount,
            uint32_t elementByteStride,
            core::ref<ReflectionType const> const& elementType,
            size_t byteSize,
            slang::TypeLayoutReflection* pSlangTypeLayout
        ) -> core::ref<ReflectionArrayType>;

        auto operator==(ReflectionArrayType const& other) const -> bool;
        auto operator==(ReflectionType const& other) const -> bool override;

        auto getElementCount() const -> uint32_t { return m_elementCount; }
        auto getElementByteStride() const -> uint32_t { return m_elementByteStride; }
        auto getElementType() const -> core::ref<ReflectionType const> { return m_elementType; }

    protected:
        ReflectionArrayType(
            uint32_t elementCount,
            uint32_t elementByteStride,
            core::ref<ReflectionType const> const& elementType,
            size_t totalByteSize,
            slang::TypeLayoutReflection* slangTypeLayout
        );

        uint32_t m_elementCount{0};
        uint32_t m_elementByteStride{0};
        core::ref<ReflectionType const> m_elementType{};
    };

    class ReflectionInterfaceType : public ReflectionType
    {
    public:
        static auto create(slang::TypeLayoutReflection* p_slangTypeLayout) -> core::ref<ReflectionInterfaceType>;

        auto operator==(ReflectionInterfaceType const& other) const -> bool;
        auto operator==(ReflectionType const& other) const -> bool override;

        auto getParameterBlockReflector() -> core::ref<const ParameterBlockReflection> const& { return mp_parameterBlockReflector; }
        auto setParameterBlockReflector(core::ref<const ParameterBlockReflection> const& p_reflector) -> void { mp_parameterBlockReflector = p_reflector; }

    protected:
        ReflectionInterfaceType(slang::TypeLayoutReflection* p_slangTypeLayout);

        core::ref<const ParameterBlockReflection> mp_parameterBlockReflector{};
    };

    class ReflectionVariable : public core::Object
    {
        APRIL_OBJECT(ReflectionVariable)
    public:
        /**
        * Create a new object
        * @param[in] name The name of the variable
        * @param[in] pType The type of the variable
        * @param[in] bindLocation The offset of the variable relative to the parent object
        */
        static auto create(std::string const& name, core::ref<const ReflectionType> const& p_type, ShaderVariableOffset const& bindLocation) -> core::ref<ReflectionVariable>;

        auto getName() const -> std::string const& { return m_name; }

        /**
        * Get the variable type
        */
        auto getType() const -> ReflectionType const* { return mp_type.get(); }

        /**
        * Get the variable offset
        */
        auto getBindLocation() const -> ShaderVariableOffset { return m_bindLocation; }
        auto getByteOffset() const -> size_t { return m_bindLocation.getByteOffset(); }
        auto getOffset() const -> size_t { return m_bindLocation.getByteOffset(); }

        bool operator==(const ReflectionVariable& other) const;
        bool operator!=(const ReflectionVariable& other) const { return !(*this == other); }

    private:
        ReflectionVariable(std::string const& name, core::ref<const ReflectionType> const& p_type, ShaderVariableOffset const& bindLocation);

        std::string m_name;
        core::ref<ReflectionType const> mp_type;
        ShaderVariableOffset m_bindLocation;
    };

    class ProgramReflection;

    /**
    * A reflection object describing a parameter block
    */
    class ParameterBlockReflection : public core::Object
    {
        APRIL_OBJECT(ParameterBlockReflection)
    public:
        static constexpr uint32_t kInvalidIndex = 0xffffffff;

        // -------------------------------------------------------------------
        // Factory Methods
        // -------------------------------------------------------------------
        static auto create(ProgramVersion const* pProgramVersion, core::ref<ReflectionType const> const& elementType) -> core::ref<ParameterBlockReflection>;
        static auto create(ProgramVersion const* pProgramVersion, slang::TypeLayoutReflection* pSlangLayout) -> core::ref<ParameterBlockReflection>;
        static auto createEmpty(ProgramVersion const* pProgramVersion) -> core::ref<ParameterBlockReflection>;

        using BindLocation = TypedShaderVariableOffset;

        // -------------------------------------------------------------------
        // Reflection Data Access
        // -------------------------------------------------------------------
        auto getElementType() const -> core::ref<ReflectionType const> { return m_elementType; }
        auto setElementType(core::ref<ReflectionType const> const& elementType) -> void;

        auto getResourceRangeCount() const -> uint32_t { return (uint32_t)m_resourceRanges.size(); }
        auto getResourceRange(uint32_t index) const -> ReflectionType::ResourceRange const& { return getElementType()->getResourceRange(index); }

        auto getResource(std::string_view name) const -> core::ref<ReflectionVariable const>;
        auto getResourceBinding(std::string_view name) const -> BindLocation;

        auto findMember(std::string_view name) const -> core::ref<ReflectionVariable const>  { return getElementType()->findMember(name); }
        auto getProgramVersion() const -> ProgramVersion const* { return m_programVersion; }

        // -------------------------------------------------------------------
        // Binding Info Structures
        // -------------------------------------------------------------------
        struct ResourceRangeBindingInfo
        {
            enum class Flavor
            {
                Simple,         ///< A simple resource range (texture/sampler/etc.)
                RootDescriptor, ///< A resource root descriptor (buffers only)
                ConstantBuffer, ///< A sub-object for a constant buffer
                ParameterBlock, ///< A sub-object for a parameter block
                Interface       ///< A sub-object for an interface-type parameter
            };

            Flavor flavor{Flavor::Simple};
            ReflectionResourceType::Dimensions dimension{ReflectionResourceType::Dimensions::Unknown};

            uint32_t regIndex{0};   ///< The register index
            uint32_t regSpace{0};   ///< The register space
            uint32_t descriptorSetIndex{kInvalidIndex}; ///< The index of the descriptor set

            core::ref<ParameterBlockReflection const> subObjectReflector;

            auto isDescriptorSet() const -> bool { return flavor == Flavor::Simple; }
            auto isRootDescriptor() const -> bool { return flavor == Flavor::RootDescriptor; }
        };

        struct DefaultConstantBufferBindingInfo
        {
            uint32_t regIndex{0};
            uint32_t regSpace{0};
            uint32_t descriptorSetIndex{kInvalidIndex};
            bool useRootConstants{false};
        };

        // -------------------------------------------------------------------
        // Construction & Setup
        // -------------------------------------------------------------------
        auto addResourceRange(ResourceRangeBindingInfo const& bindingInfo) -> void;
        friend struct ParameterBlockReflectionFinalizer;
        auto finalize() -> void;

        auto hasDefaultConstantBuffer() const -> bool;
        auto setDefaultConstantBufferBindingInfo(DefaultConstantBufferBindingInfo const& info) -> void;
        auto getDefaultConstantBufferBindingInfo() const -> DefaultConstantBufferBindingInfo const&;

        auto getResourceRangeBindingInfo(uint32_t index) const -> ResourceRangeBindingInfo const& { return m_resourceRanges[index]; }

        // -------------------------------------------------------------------
        // Sub-object Accessors
        // -------------------------------------------------------------------
        auto getRootDescriptorRangeCount() const -> uint32_t { return (uint32_t)m_rootDescriptorRangeIndices.size(); }
        auto getRootDescriptorRangeIndex(uint32_t index) const -> uint32_t { return m_rootDescriptorRangeIndices[index]; }

        auto getParameterBlockSubObjectRangeCount() const -> uint32_t { return (uint32_t)m_parameterBlockSubObjectRangeIndices.size(); }
        auto getParameterBlockSubObjectRangeIndex(uint32_t index) const -> uint32_t { return m_parameterBlockSubObjectRangeIndices[index]; }

    protected:
        ParameterBlockReflection(ProgramVersion const* pProgramVersion);

    private:

        core::ref<ReflectionType const> m_elementType{};

        /// Binding information for the resource ranges in the element type.
        std::vector<ResourceRangeBindingInfo> m_resourceRanges{};

        /// Binding information for the "default" constant buffer.
        DefaultConstantBufferBindingInfo m_defaultConstantBufferBindingInfo{};

        /// Indices of resource ranges that represent root descriptors.
        std::vector<uint32_t> m_rootDescriptorRangeIndices{};

        /// Indices of resource ranges that represent parameter blocks.
        std::vector<uint32_t> m_parameterBlockSubObjectRangeIndices{};

        ProgramVersion const* m_programVersion{nullptr};
    };

    // --------------------------------------------------------------------------
    // EntryPointGroupReflection
    // --------------------------------------------------------------------------
    class EntryPointGroupReflection : public ParameterBlockReflection
    {
        APRIL_OBJECT(EntryPointGroupReflection)
    public:
        static auto create(
            ProgramVersion const* pProgramVersion,
            uint32_t groupIndex,
            std::vector<slang::EntryPointLayout*> const& slangEntryPointReflectors
        ) -> core::ref<EntryPointGroupReflection>;

    private:
        EntryPointGroupReflection(ProgramVersion const* pProgramVersion);
    };

    using EntryPointBaseReflection = EntryPointGroupReflection;

    // --------------------------------------------------------------------------
    // ProgramReflection
    // --------------------------------------------------------------------------
    /**
     * Reflection object for an entire program. Essentially, it's a collection of ParameterBlocks.
     */
    class ProgramReflection : public core::Object
    {
        APRIL_OBJECT(ProgramReflection)
    public:
        /**
         * Data structure describing a shader input/output variable.
         * Used mostly to communicate VS inputs and PS outputs.
         */
        struct ShaderVariable
        {
            uint32_t bindLocation{0};                                               ///< The bind-location of the variable
            std::string semanticName{};                                             ///< The semantic name of the variable
            ReflectionBasicType::Type type{ReflectionBasicType::Type::Unknown};     ///< The type of the variable
        };

        using VariableMap = std::map<std::string, ShaderVariable, std::less<>>;
        using BindLocation = ParameterBlockReflection::BindLocation;

        /**
         * Data structure describing a hashed string used in the program.
         */
        struct HashedString
        {
            uint32_t hash{0};
            std::string string{};
        };

        /**
         * Create a new object for a Slang reflector object.
         */
        static auto create(
            ProgramVersion const* pProgramVersion,
            slang::ShaderReflection* pSlangReflector,
            std::vector<slang::EntryPointLayout*> const& slangEntryPointReflectors,
            std::string& log
        ) -> core::ref<ProgramReflection const>;

        auto finalize() -> void;

        auto getProgramVersion() const -> ProgramVersion const* { return m_programVersion; }

        /**
         * Get parameter block by name.
         */
        auto getParameterBlock(std::string_view name) const -> core::ref<ParameterBlockReflection const>;

        /**
         * Get the default (unnamed) parameter block.
         */
        auto getDefaultParameterBlock() const -> core::ref<ParameterBlockReflection const> { return m_defaultBlock; }

        /**
         * For compute-shaders, return the required thread-group size.
         */
        auto getThreadGroupSize() const -> uint3 { return m_threadGroupSize; }

        /**
         * For pixel-shaders, check if we need to run the shader at sample frequency.
         */
        auto isSampleFrequency() const -> bool { return m_isSampleFrequency; }

        /**
         * Get a resource from the default parameter block.
         */
        auto getResource(std::string_view name) const -> core::ref<ReflectionVariable const>;

        /**
         * Search for a vertex attribute by its semantic name.
         */
        auto getVertexAttributeBySemantic(std::string_view semantic) const -> ShaderVariable const*;

        /**
         * Search for a vertex attribute by the variable name.
         */
        auto getVertexAttribute(std::string_view name) const -> ShaderVariable const*;

        /**
         * Get a pixel shader output variable.
         */
        auto getPixelShaderOutput(std::string_view name) const -> ShaderVariable const*;

        /**
         * Look up a type by name.
         * @return nullptr if the type does not exist.
         */
        auto findType(std::string_view name) const -> core::ref<ReflectionType>;

        auto findMember(std::string_view name) const -> core::ref<ReflectionVariable const>;

        auto getEntryPointGroups() const -> std::vector<core::ref<EntryPointGroupReflection>> const& { return m_entryPointGroups; }

        auto getEntryPointGroup(uint32_t index) const -> core::ref<EntryPointGroupReflection> const& { return m_entryPointGroups[index]; }

        auto getHashedStrings() const -> std::vector<HashedString> const& { return m_hashedStrings; }

    private:
        ProgramReflection(
            ProgramVersion const* pProgramVersion,
            slang::ShaderReflection* pSlangReflector,
            std::vector<slang::EntryPointLayout*> const& slangEntryPointReflectors,
            std::string& log
        );
        ProgramReflection(ProgramVersion const* pProgramVersion);
        ProgramReflection(ProgramReflection const&) = default;

        auto setDefaultParameterBlock(core::ref<ParameterBlockReflection> const& pBlock) -> void;

        ProgramVersion const* m_programVersion{nullptr};

        core::ref<ParameterBlockReflection> m_defaultBlock{};
        uint3 m_threadGroupSize{};
        bool m_isSampleFrequency{false};

        VariableMap m_psOut{};
        VariableMap m_vertAttr{};
        VariableMap m_vertAttrBySemantic{};

        slang::ShaderReflection* m_slangReflector{nullptr}; // Raw pointer from Slang (non-owning usually, or kept alive by session)

        mutable std::map<std::string, core::ref<ReflectionType>, std::less<>> m_mapNameToType{};

        std::vector<core::ref<EntryPointGroupReflection>> m_entryPointGroups{};

        std::vector<HashedString> m_hashedStrings{};
    };

    AP_ENUM_REGISTER(ReflectionType::Kind);
    AP_ENUM_REGISTER(ReflectionBasicType::Type);
    AP_ENUM_REGISTER(ReflectionResourceType::ShaderAccess);
    AP_ENUM_REGISTER(ReflectionResourceType::ReturnType);
    AP_ENUM_REGISTER(ReflectionResourceType::Dimensions);
    AP_ENUM_REGISTER(ReflectionResourceType::StructuredType);
    AP_ENUM_REGISTER(ReflectionResourceType::Type);
}
