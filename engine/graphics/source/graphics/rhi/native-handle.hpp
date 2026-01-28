// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include <core/log/logger.hpp>
#include <cstdint>
#include <slang-gfx.h>

namespace april::graphics
{
    enum class NativeHandleType
    {
        Unknown,

        ID3D12Device,
        ID3D12Resource,
        ID3D12PipelineState,
        ID3D12Fence,
        ID3D12CommandQueue,
        ID3D12GraphicsCommandList,
        D3D12_CPU_DESCRIPTOR_HANDLE,

        VkInstance,
        VkPhysicalDevice,
        VkDevice,
        VkImage,
        VkImageView,
        VkBuffer,
        VkBufferView,
        VkPipeline,
        VkFence,
        VkQueue,
        VkCommandBuffer,
        VkSampler,
    };

    template <typename T>
struct NativeHandleTrait
{};

#define APRIL_NATIVE_HANDLE(T, TYPE)                \
    template <>                                       \
    struct NativeHandleTrait<T>                      \
    {                                                \
        static const NativeHandleType type = TYPE;   \
        static uint64_t pack(T native)               \
        {                                            \
            return fstd::bit_cast<uint64_t>(native); \
        }                                            \
        static T unpack(uint64_t value)              \
        {                                            \
            return fstd::bit_cast<T>(value);         \
        }                                            \
    };

#if APRIL_HAS_D3D12
    APRIL_NATIVE_HANDLE(ID3D12Device*, NativeHandleType::ID3D12Device);
    APRIL_NATIVE_HANDLE(ID3D12Resource*, NativeHandleType::ID3D12Resource);
    APRIL_NATIVE_HANDLE(ID3D12PipelineState*, NativeHandleType::ID3D12PipelineState);
    APRIL_NATIVE_HANDLE(ID3D12Fence*, NativeHandleType::ID3D12Fence);
    APRIL_NATIVE_HANDLE(ID3D12CommandQueue*, NativeHandleType::ID3D12CommandQueue);
    APRIL_NATIVE_HANDLE(ID3D12GraphicsCommandList*, NativeHandleType::ID3D12GraphicsCommandList);
    APRIL_NATIVE_HANDLE(D3D12_CPU_DESCRIPTOR_HANDLE, NativeHandleType::D3D12_CPU_DESCRIPTOR_HANDLE);
#endif // APRIL_HAS_D3D12

#if APRIL_HAS_VULKAN
    APRIL_NATIVE_HANDLE(VkInstance, NativeHandleType::VkInstance);
    APRIL_NATIVE_HANDLE(VkPhysicalDevice, NativeHandleType::VkPhysicalDevice);
    APRIL_NATIVE_HANDLE(VkDevice, NativeHandleType::VkDevice);
    APRIL_NATIVE_HANDLE(VkImage, NativeHandleType::VkImage);
    APRIL_NATIVE_HANDLE(VkImageView, NativeHandleType::VkImageView);
    APRIL_NATIVE_HANDLE(VkBuffer, NativeHandleType::VkBuffer);
    APRIL_NATIVE_HANDLE(VkBufferView, NativeHandleType::VkBufferView);
    APRIL_NATIVE_HANDLE(VkPipeline, NativeHandleType::VkPipeline);
    APRIL_NATIVE_HANDLE(VkFence, NativeHandleType::VkFence);
    APRIL_NATIVE_HANDLE(VkQueue, NativeHandleType::VkQueue);
    APRIL_NATIVE_HANDLE(VkCommandBuffer, NativeHandleType::VkCommandBuffer);
    APRIL_NATIVE_HANDLE(VkSampler, NativeHandleType::VkSampler);
#endif // APRIL_HAS_VULKAN

#undef APRIL_NATIVE_HANDLE

    /// Represents a native graphics API handle (e.g. D3D12 or Vulkan).
    /// Native handles are expected to fit into 64 bits.
    class NativeHandle
    {
    public:
        NativeHandle() = default;

        template <typename T>
        explicit NativeHandle(T native)
        {
            // TODO:
            // m_type = NativeHandleTrait<T>::type;
            // m_value = NativeHandleTrait<T>::pack(native);
        }

        auto getType() const -> NativeHandleType { return m_type; }

        auto isValid() const -> bool { return m_type != NativeHandleType::Unknown; }

        template <typename T>
        auto as() const -> T
        {
            if (m_type != NativeHandleTrait<T>::type)
            {
                AP_CRITICAL("Invalid native handle cast!");
            }
            return NativeHandleTrait<T>::unpack(m_value);
        }

    private:
        NativeHandleType m_type{NativeHandleType::Unknown};
        uint64_t m_value{0};
    };

} // namespace april::graphics
