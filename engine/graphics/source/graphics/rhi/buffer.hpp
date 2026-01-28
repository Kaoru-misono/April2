// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "resource.hpp"

#include <core/foundation/object.hpp>
#include <core/tools/enum.hpp>
#include <core/math/type.hpp>

#include <vector>
#include <string>

namespace april::graphics
{
    class Program;
    class ShaderVariable;

    namespace detail
    {
        /**
         * Helper for converting host type to resource format for typed buffers.
         * See list of supported formats for typed UAV loads:
         * https://docs.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads
         */
        template <typename T>
        struct FormatForElementType
        {};

        #define CASE(TYPE, FORMAT)                                \
            template <>                                            \
            struct FormatForElementType<TYPE>                     \
            {                                                     \
                static constexpr ResourceFormat kFormat = FORMAT; \
            }

        // Guaranteed supported formats on D3D12.
        CASE(float, ResourceFormat::R32Float);
        CASE(uint32_t, ResourceFormat::R32Uint);
        CASE(int32_t, ResourceFormat::R32Int);

        // Optionally supported formats as a set on D3D12. If one is supported all are supported.
        CASE(float4, ResourceFormat::RGBA32Float);
        CASE(uint4, ResourceFormat::RGBA32Uint);
        CASE(int4, ResourceFormat::RGBA32Int);
        // Need vector types to support these.

        CASE(uint16_t, ResourceFormat::R16Uint);
        CASE(int16_t, ResourceFormat::R16Int);

        CASE(uint8_t, ResourceFormat::R8Uint);
        CASE(int8_t, ResourceFormat::R8Int);

        // Optionally and individually supported formats on D3D12. Query for support individually.
        CASE(float2, ResourceFormat::RG32Float);
        CASE(uint2, ResourceFormat::RG32Uint);
        CASE(int2, ResourceFormat::RG32Int);

        // Additional formats that may be supported on some hardware.
        CASE(float3, ResourceFormat::RGB32Float);

        #undef CASE
    } // namespace detail

    /// Buffer memory types.
    enum class MemoryType : uint32_t
    {
        DeviceLocal, ///< Device local memory. The buffer can be updated using Buffer::setBlob().
        Upload,      ///< Upload memory. The buffer can be mapped for CPU writes.
        ReadBack,    ///< Read-back memory. The buffer can be mapped for CPU reads.
    };
    AP_ENUM_INFO(
        MemoryType,
        {
            {MemoryType::DeviceLocal, "DeviceLocal"},
            {MemoryType::Upload, "Upload"},
            {MemoryType::ReadBack, "ReadBack"},
        }
    );
    AP_ENUM_REGISTER(MemoryType);

    enum class BufferUsage: uint32_t
    {
        None = 0,
        VertexBuffer = (1 << 0),
        IndexBuffer = (1 << 1),
        ConstantBuffer = (1 << 2),
        ShaderResource = (1 << 3),
        UnorderedAccess = (1 << 4),
        IndirectArgument = (1 << 5),
        CopySource = (1 << 6),
        CopyDestination = (1 << 7),
        AccelerationStructure = (1 << 8),
        AccelerationStructureBuildInput = (1 << 9),
        ShaderTable = (1 << 10),
        Shared = (1 << 11),
    };
    AP_ENUM_CLASS_OPERATORS(BufferUsage);

    /**
     * Low-level buffer object
     * This class abstracts the API's buffer creation and management
     */
    class Buffer : public Resource
    {
        APRIL_OBJECT(Buffer)
    public:
        static constexpr uint64_t kEntireBuffer = ResourceViewInfo::kEntireBuffer;

        /// Constructor.
        Buffer(
            core::ref<Device> p_device,
            size_t size,
            size_t structSize,
            ResourceFormat format,
            BufferUsage usage,
            MemoryType memoryType,
            void const* pInitData
        );

        /// Constructor for raw buffer.
        Buffer(core::ref<Device> p_device, size_t size, BufferUsage usage, MemoryType memoryType, void const* pInitData);

        /// Constructor for typed buffer.
        Buffer(
            core::ref<Device> p_device,
            ResourceFormat format,
            uint32_t elementCount,
            BufferUsage usage,
            MemoryType memoryType,
            void const* pInitData
        );

        /// Constructor for structured buffer.
        Buffer(
            core::ref<Device> p_device,
            uint32_t structSize,
            uint32_t elementCount,
            BufferUsage usage,
            MemoryType memoryType,
            void const* pInitData,
            bool createCounter
        );

        /// Constructor with existing resource.
        Buffer(core::ref<Device> p_device, rhi::IBuffer* pResource, size_t size, BufferUsage usage, MemoryType memoryType);

        /// Constructor with native handle.
        Buffer(core::ref<Device> p_device, NativeHandle handle, size_t size, BufferUsage usage, MemoryType memoryType);

        /// Destructor.
        ~Buffer();

        auto getGfxBufferResource() const -> rhi::IBuffer* { return m_gfxBuffer; }

        auto getGfxResource() const -> rhi::IResource* override;

        /**
         * Get a shader-resource view.
         * @param[in] offset Offset in bytes.
         * @param[in] size Size in bytes.
         */
        auto getSRV(uint64_t offset, uint64_t size = kEntireBuffer) -> core::ref<ShaderResourceView>;

        /**
         * Get an unordered access view.
         * @param[in] offset Offset in bytes.
         * @param[in] size size in bytes.
         */
        auto getUAV(uint64_t offset, uint64_t size = kEntireBuffer) -> core::ref<UnorderedAccessView>;

        /**
         * Get a shader-resource view for the entire resource
         */
        auto getSRV() -> core::ref<ShaderResourceView> override;

        /**
         * Get an unordered access view for the entire resource
         */
        auto getUAV() -> core::ref<UnorderedAccessView> override;

        /**
         * Update the buffer's data
         * @param[in] pData Pointer to the source data.
         * @param[in] offset Byte offset into the destination buffer, indicating where to start copy into.
         * @param[in] size Number of bytes to copy.
         * Throws an exception if data causes out-of-bound access to the buffer.
         */
        virtual auto setBlob(void const* pData, size_t offset, size_t size) -> void;

        /**
         * Read the buffer's data
         * @param pData Pointer to the destination data.
         * @param offset Byte offset into the source buffer, indicating where to start copy from.
         * @param size Number of bytes to copy.
         */
        auto getBlob(void* pData, size_t offset, size_t size) const -> void;

        /**
         * Get the GPU address (this includes the offset)
         */
        auto getGpuAddress() const -> uint64_t;

        /**
         * Get the size of the buffer in bytes.
         */
        // getSize() is inherited from Resource

        /**
         * Get the element count. For structured-buffers, this is the number of structs. For typed-buffers, this is the number of elements. For
         * other buffer, will return the size in bytes.
         */
        auto getElementCount() const -> uint32_t { return m_elementCount; }

        /**
         * Get the size of a single struct. This call is only valid for structured-buffer. For other buffer types, it will return 0
         */
        auto getStructSize() const -> uint32_t { return m_structSize; }

        /**
         * Get the buffer format. This call is only valid for typed-buffers, for other buffer types it will return ResourceFormat::Unknown
         */
        auto getFormat() const -> ResourceFormat { return m_format; }

        /**
         * Get the UAV counter buffer
         */
        auto getUAVCounter() const -> core::ref<Buffer> const& { return m_uavCounter; }

        /**
         * Map the buffer.
         */
        auto map(rhi::CpuAccessMode mode = rhi::CpuAccessMode::Write) const -> void*;

        template <typename T> requires (!std::is_pointer_v<T>)
        auto mapAs(rhi::CpuAccessMode mode = rhi::CpuAccessMode::Write) const -> T*
        {
            return static_cast<T*>(map(mode));
        }

        /**
         * Unmap the buffer
         */
        auto unmap() const -> void;

        /**
         * Get safe offset and size values
         */
        auto adjustSizeOffsetParams(size_t& size, size_t& offset) const -> bool;

        /**
         * Get the memory type
         */
        auto getMemoryType() const -> MemoryType { return m_memoryType; }

        /**
         * Get the buffer usage
         */
        auto getUsage() const -> BufferUsage { return m_usage; }

        /**
         * Check if this is a typed buffer
         */
        auto isTyped() const -> bool { return m_format != ResourceFormat::Unknown; }

        /**
         * Check if this is a structured-buffer
         */
        auto isStructured() const -> bool { return m_structSize != 0; }

        template <typename T>
        auto setElement(uint32_t index, T const& value) -> void
        {
            setBlob(&value, sizeof(T) * index, sizeof(T));
        }

        template <typename T>
        auto getElements(uint32_t firstElement = 0, uint32_t elementCount = 0) const -> std::vector<T>
        {
            if (elementCount == 0) elementCount = (m_size / sizeof(T)) - firstElement;

            std::vector<T> data(elementCount);
            getBlob(data.data(), firstElement * sizeof(T), elementCount * sizeof(T));

            return data;
        }

        template <typename T>
        auto getElement(uint32_t index) const -> T
        {
            T data;
            getBlob(&data, index * sizeof(T), sizeof(T));
            return data;
        }

    protected:
        Slang::ComPtr<rhi::IBuffer> m_gfxBuffer{};

        MemoryType m_memoryType{};
        BufferUsage m_usage{BufferUsage::None};
        uint32_t m_elementCount{0};
        ResourceFormat m_format{ResourceFormat::Unknown};
        uint32_t m_structSize{0};
        core::ref<Buffer> m_uavCounter{}; // For structured-buffers
        mutable void* m_mappedPtr{nullptr};
    };

    inline auto to_string(MemoryType type) -> std::string
    {
        #define item_to_string(item) \
           case MemoryType::item:  \
                return #item;
            switch (type)
            {
                item_to_string(DeviceLocal);
                item_to_string(Upload);
                item_to_string(ReadBack);
            default:
                AP_UNREACHABLE();
            }
        #undef item_to_string
    }

    inline auto to_string(BufferUsage usages) -> std::string
    {
        std::string s;
        if (usages == BufferUsage::None)
        {
            return "None";
        }

    #define item_to_string(item)                       \
        if (enum_has_any_flags(usages, BufferUsage::item)) \
        (s += (s.size() ? " | " : "") + std::string(#item))

        item_to_string(VertexBuffer);
        item_to_string(IndexBuffer);
        item_to_string(ConstantBuffer);
        item_to_string(ShaderResource);
        item_to_string(UnorderedAccess);
        item_to_string(IndirectArgument);
        item_to_string(CopySource);
        item_to_string(CopyDestination);
        item_to_string(AccelerationStructure);
        item_to_string(AccelerationStructureBuildInput);
        item_to_string(ShaderTable);
        item_to_string(Shared);

    #undef item_to_string

        return s;
    }

} // namespace april::graphics
