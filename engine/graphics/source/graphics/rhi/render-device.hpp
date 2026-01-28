// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "types.hpp"
#include "texture.hpp"
#include "sampler.hpp"
#include "native-handle.hpp"
#include "format.hpp"
#include "query-heap.hpp"
#include "gpu-memory-heap.hpp"
#include "resource.hpp"
#include "buffer.hpp"
#include "program/program-reflection.hpp"

#include <core/foundation/object.hpp>
#include <core/tools/enum.hpp>

#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-rhi.h>

#include <array>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>
#include <string>

namespace april::core { class Profiler; }

namespace april::graphics
{
    class ProgramManager;
    class GpuProfiler;
    class AftermathContext;
    class ShaderVariable;

    namespace cuda_utils
    {
        class CudaDevice;
    }

    /// Holds the adapter LUID (or UUID).
    struct AdapterLUID
    {
        std::array<uint8_t, 16> luid;

        AdapterLUID() { luid.fill(0); }
        auto isValid() const -> bool { return *this != AdapterLUID(); }
        auto operator==(AdapterLUID const& other) const -> bool { return luid == other.luid; }
        auto operator!=(AdapterLUID const& other) const -> bool { return luid != other.luid; }
        auto operator<(AdapterLUID const& other) const -> bool { return luid < other.luid; }
    };

    struct AdapterInfo
    {
        /// Descriptive name of the adapter.
        std::string name;

        /// Unique identifier for the vendor.
        uint32_t vendorID;

        // Unique identifier for the physical device among devices from the vendor.
        uint32_t deviceID;

        // Logically unique identifier of the adapter.
        AdapterLUID luid;
    };

    class Device : public core::Object
    {
        APRIL_OBJECT(Device)
    public:
        /**
         * Maximum number of in-flight frames.
         */
        static constexpr uint32_t kInFlightFrameCount = 3;

        /// Device type.
        enum Type
        {
            Default, ///< Default device type, favors D3D12 over Vulkan.
            D3D12,
            Vulkan,
        };
        AP_ENUM_INFO(
            Type,
            {
                {Type::Default, "Default"},
                {Type::D3D12, "D3D12"},
                {Type::Vulkan, "Vulkan"},
            }
        );

        /// Device descriptor.
        struct Desc
        {
            /// The device type (D3D12/Vulkan).
            Type type{Type::Default};

            /// GPU index (indexing into GPU list returned by getGPUList()).
            uint32_t gpu{0};

            /// Enable the debug layer. The default for release build is false, for debug build it's true.
            bool enableDebugLayer{false};

            /// Enable NVIDIA NSight Aftermath GPU crash dump.
            bool enableAftermath{false};

            /// The maximum number of entries allowable in the shader cache. A value of 0 indicates no limit.
            uint32_t maxShaderCacheEntryCount{1000};

            /// The full path to the root directory for the shader cache. An empty string will disable the cache.
            std::string shaderCachePath;

            /// Whether to enable ray tracing validation (requires NVAPI)
            bool enableRaytracingValidation{false};
        };

        struct Info
        {
            std::string adapterName;
            AdapterLUID adapterLUID;
            std::string apiName;
        };

        struct Limits
        {
            uint3 maxComputeDispatchThreadGroups;
            uint32_t maxShaderVisibleSamplers;
        };

        struct CacheStats
        {
            uint32_t hitCount;
            uint32_t missCount;
            uint32_t entryCount;
        };

        enum class SupportedFeatures : uint64_t
        {
            None = 0x0,
            ProgrammableSamplePositionsPartialOnly = 0x1,
            ProgrammableSamplePositionsFull = 0x2,
            Barycentrics = 0x4,
            Raytracing = 0x8,
            RaytracingTier1_1 = 0x10,
            ConservativeRasterizationTier1 = 0x20,
            ConservativeRasterizationTier2 = 0x40,
            ConservativeRasterizationTier3 = 0x80,
            RasterizerOrderedViews = 0x100,
            WaveOperations = 0x200,
            ShaderExecutionReorderingAPI = 0x400,
            RaytracingReordering = 0x800,
        };

        /**
         * Constructor. Throws an exception if creation failed.
         * @param[in] desc Device configuration descriptor.
         */
        Device(Desc const& desc);
        virtual ~Device();

        /**
         * Create a new buffer.
         */
        auto createBuffer(
            size_t size,
            BufferUsage usage = BufferUsage::ShaderResource | BufferUsage::UnorderedAccess,
            MemoryType memoryType = MemoryType::DeviceLocal,
            void const* pInitData = nullptr
        ) -> core::ref<Buffer>;

        /**
         * Create a new typed buffer.
         */
        auto createTypedBuffer(
            ResourceFormat format,
            uint32_t elementCount,
            BufferUsage usage = BufferUsage::ShaderResource | BufferUsage::UnorderedAccess,
            MemoryType memoryType = MemoryType::DeviceLocal,
            void const* pInitData = nullptr
        ) -> core::ref<Buffer>;

        /**
         * Create a new typed buffer. The format is deduced from the template parameter.
         */
        template <typename T>
        auto createTypedBuffer(
            uint32_t elementCount,
            BufferUsage usage = BufferUsage::ShaderResource | BufferUsage::UnorderedAccess,
            MemoryType memoryType = MemoryType::DeviceLocal,
            T const* pInitData = nullptr
        ) -> core::ref<Buffer>
        {
            return createTypedBuffer(detail::FormatForElementType<T>::kFormat, elementCount, usage, memoryType, pInitData);
        }

        /**
         * Create a new structured buffer.
         */
        auto createStructuredBuffer(
            uint32_t structSize,
            uint32_t elementCount,
            BufferUsage usage = BufferUsage::ShaderResource | BufferUsage::UnorderedAccess,
            MemoryType memoryType = MemoryType::DeviceLocal,
            void const* pInitData = nullptr,
            bool createCounter = false
        ) -> core::ref<Buffer>;

        /**
         * Create a new structured buffer.
         */
        auto createStructuredBuffer(
            ReflectionType const* pType,
            uint32_t elementCount,
            BufferUsage usage = BufferUsage::ShaderResource | BufferUsage::UnorderedAccess,
            MemoryType memoryType = MemoryType::DeviceLocal,
            void const* pInitData = nullptr,
            bool createCounter = false
        ) -> core::ref<Buffer>;

        /**
         * Create a new structured buffer.
         */
        auto createStructuredBuffer(
            ShaderVariable const& shaderVariable,
            uint32_t elementCount,
            BufferUsage usage = BufferUsage::ShaderResource | BufferUsage::UnorderedAccess,
            MemoryType memoryType = MemoryType::DeviceLocal,
            void const* pInitData = nullptr,
            bool createCounter = false
        ) -> core::ref<Buffer>;

        /**
         * Create a new buffer from an existing resource.
         */
        auto createBufferFromResource(rhi::IBuffer* pResource, size_t size, BufferUsage usage, MemoryType memoryType) -> core::ref<Buffer>;

        /**
         * Create a new buffer from an existing native handle.
         */
        auto createBufferFromNativeHandle(NativeHandle handle, size_t size, BufferUsage usage, MemoryType memoryType) -> core::ref<Buffer>;

        /**
         * Create a 1D texture.
         */
        auto createTexture1D(
            uint32_t width,
            ResourceFormat format,
            uint32_t arraySize = 1,
            uint32_t mipLevels = Resource::kMaxPossible,
            void const* pInitData = nullptr,
            TextureUsage usage = TextureUsage::ShaderResource
        ) -> core::ref<Texture>;

        /**
         * Create a 2D texture.
         */
        auto createTexture2D(
            uint32_t width,
            uint32_t height,
            ResourceFormat format,
            uint32_t arraySize = 1,
            uint32_t mipLevels = Resource::kMaxPossible,
            void const* pInitData = nullptr,
            TextureUsage usage = TextureUsage::ShaderResource
        ) -> core::ref<Texture>;

        /**
         * Create a 3D texture.
         */
        auto createTexture3D(
            uint32_t width,
            uint32_t height,
            uint32_t depth,
            ResourceFormat format,
            uint32_t mipLevels = Resource::kMaxPossible,
            void const* pInitData = nullptr,
            TextureUsage usage = TextureUsage::ShaderResource
        ) -> core::ref<Texture>;

        /**
         * Create a cube texture.
         */
        auto createTextureCube(
            uint32_t width,
            uint32_t height,
            ResourceFormat format,
            uint32_t arraySize = 1,
            uint32_t mipLevels = Resource::kMaxPossible,
            void const* pInitData = nullptr,
            TextureUsage usage = TextureUsage::ShaderResource
        ) -> core::ref<Texture>;

        /**
         * Create a multi-sampled 2D texture.
         */
        auto createTexture2DMS(
            uint32_t width,
            uint32_t height,
            ResourceFormat format,
            uint32_t sampleCount,
            uint32_t arraySize = 1,
            TextureUsage usage = TextureUsage::ShaderResource
        ) -> core::ref<Texture>;

        /**
         * Create a new texture from an resource.
         */
        auto createTextureFromResource(
            rhi::ITexture* pResource,
            Texture::Type type,
            ResourceFormat format,
            uint32_t width,
            uint32_t height,
            uint32_t depth,
            uint32_t arraySize,
            uint32_t mipLevels,
            uint32_t sampleCount,
            TextureUsage usage,
            Resource::State initState
        ) -> core::ref<Texture>;

        /**
         * Create a new graphics heap.
         */
        auto createHeap(rhi::HeapDesc const& desc) -> Slang::ComPtr<rhi::IHeap>;

        /**
         * Create a new sampler object.
         */
        auto createSampler(Sampler::Desc const& desc) -> core::ref<Sampler>;

        /**
         * Create a new fence object.
         */
        auto createFence(FenceDesc const& desc) -> core::ref<Fence>;

        /**
         * Create a new fence object.
         */
        auto createFence(bool shared = false) -> core::ref<Fence>;

        /// Create a compute pipeline.
        auto createComputePipeline(ComputePipelineDesc const& desc) -> core::ref<ComputePipeline>;
        /// Create a graphics pipeline.
        auto createGraphicsPipeline(GraphicsPipelineDesc const& desc) -> core::ref<GraphicsPipeline>;
        /// Create a raytracing pipeline.
        auto createRayTracingPipeline(RayTracingPipelineDesc const& desc) -> core::ref<RayTracingPipeline>;

        auto getProgramManager() const -> ProgramManager*;

        auto getProfiler() const -> core::Profiler*;
        auto getGpuProfiler() const -> GpuProfiler*;

        auto getShaderCacheStats() const -> CacheStats;
        auto getPipelineCacheStats() const -> CacheStats;

        /**
         * Get the default command-context.
         */
        auto getCommandContext() const -> CommandContext*;

        /// Returns the global slang session.
        auto getSlangGlobalSession() const -> slang::IGlobalSession* { return m_slangGlobalSession; }

        /// Return the GFX define.
        auto getGfxDevice() const -> rhi::IDevice* { return m_gfxDevice; }

        /// Return the GFX command queue.
        auto getGfxCommandQueue() const -> rhi::ICommandQueue* { return m_gfxCommandQueue; }

        /**
         * Returns the native API handle:
         * - D3D12: ID3D12Device* (0)
         * - Vulkan: VkInstance (0), VkPhysicalDevice (1), VkDevice (2)
         */
        auto getNativeHandle(uint32_t index = 0) const -> rhi::NativeHandle;

        /**
         * End a frame.
         */
        auto endFrame() -> void;

        /**
         * Flushes pipeline, releases resources, and blocks until completion
         */
        auto wait() -> void;

        /**
         * Get the desc
         */
        auto getDesc() const -> Desc const& { return m_desc; }

        /**
         * Get the device type.
         */
        auto getType() const -> Type { return m_desc.type; }

        /**
         * Throws an exception if the device is not a D3D12 device.
         */
        auto requireD3D12() const -> void;

        /**
         * Throws an exception if the device is not a Vulkan device.
         */
        auto requireVulkan() const -> void;

        /**
         * Get an object that represents a default sampler.
         */
        auto getDefaultSampler() const -> core::ref<Sampler> const& { return mp_defaultSampler; }

        auto getBufferDataAlignment(BufferUsage usage) -> size_t;

        auto getUploadHeap() const -> core::ref<GpuMemoryHeap> const& { return mp_uploadHeap; }
        auto getReadBackHeap() const -> core::ref<GpuMemoryHeap> const& { return mp_readBackHeap; }
        auto getTimestampQueryHeap() const -> core::ref<QueryHeap> const& { return mp_timestampQueryHeap; }
        auto releaseResource(ISlangUnknown* pResource) -> void;

        auto getGpuTimestampFrequency() const -> uint64_t { return m_gpuTimestampFrequency; }

        auto getInfo() const -> Info const& { return m_info; }

        /**
         * Get the device limits.
         */
        auto getLimits() const -> Limits const& { return m_limits; }

        /**
         * Check if features are supported by the device
         */
        auto isFeatureSupported(SupportedFeatures flags) const -> bool;

        /**
         * Check if a shader model is supported by the device
         */
        auto isShaderModelSupported(ShaderModel shaderModel) const -> bool;

        /**
         * Return the highest supported shader model by the device
         */
        auto getSupportedShaderModel() const -> ShaderModel { return m_supportedShaderModel; }

        /**
         * Return the default shader model to use
         */
        auto getDefaultShaderModel() const -> ShaderModel { return m_defaultShaderModel; }

        // auto getCurrentTransientResourceHeap() -> rhi::ITransientResourceHeap*;

        /// Get the texture row memory alignment in bytes.
        auto getTextureRowAlignment() const -> size_t;

        /// Report live objects in GFX.
        static auto reportLiveObjects() -> void;

        /**
         * Try to enable D3D12 Agility SDK at runtime.
         */
        static auto enableAgilitySDK() -> bool;

        /**
         * Get a list of all available GPUs.
         */
        static auto getGPUs(Type deviceType) -> std::vector<AdapterInfo>;

        /**
        * Get the global device mutex.
        * WARNING: Falcor is generally not thread-safe. This mutex is used in very specific
        * places only, currently only for doing parallel texture loading.
        */
        auto getGlobalGfxMutex() -> std::mutex& { return m_globalGfxMutex; }

        /**
        * When ray tracing validation is enabled, call this to force validation messages to be flushed.
        * It is automatically called at the end of each frame. Only messages from completed work
        * will flush, so to guaruntee all are printed, it must be called after a fence.
        * NOTE: This has no effect on Vulkan, in which the driver flushes automatically at device idle/lost.
        */
        auto flushRaytracingValidation() -> void;

        auto getGlobalFence() const -> core::ref<Fence> const& { return mp_frameFence; }

    private:
        struct ResourceRelease
        {
            uint64_t fenceValue;
            Slang::ComPtr<ISlangUnknown> object;
        };
        std::queue<ResourceRelease> m_deferredReleases;

        auto executeDeferredReleases() -> void;

        auto enableRaytracingValidation() -> void;
        auto disableRaytracingValidation() -> void;

        std::unique_ptr<rhi::IDebugCallback> m_callback{};
        std::unique_ptr<rhi::IPersistentCache> m_shaderCache{};
        std::unique_ptr<rhi::IPersistentCache> m_pipelineCache{};

        Desc m_desc{};
        Slang::ComPtr<slang::IGlobalSession> m_slangGlobalSession{};
        Slang::ComPtr<rhi::IDevice> m_gfxDevice{};
        Slang::ComPtr<rhi::ICommandQueue> m_gfxCommandQueue{};
        // Slang::ComPtr<rhi::ITransientResourceHeap> m_transientResourceHeaps[kInFlightFrameCount];
        uint32_t m_currentTransientResourceHeapIndex{0};

        core::ref<Sampler> mp_defaultSampler{};
        core::ref<GpuMemoryHeap> mp_uploadHeap{};
        core::ref<GpuMemoryHeap> mp_readBackHeap{};
        core::ref<QueryHeap> mp_timestampQueryHeap{};

        core::ref<Fence> mp_frameFence;

        std::unique_ptr<CommandContext> mp_commandContext;
        uint64_t m_gpuTimestampFrequency{};

        Info m_info;
        Limits m_limits;
        SupportedFeatures m_supportedFeatures{SupportedFeatures::None};
        ShaderModel m_supportedShaderModel{ShaderModel::Unknown};
        ShaderModel m_defaultShaderModel{ShaderModel::Unknown};

        std::unique_ptr<ProgramManager> mp_programManager;
        core::ref<GpuProfiler> mp_gpuProfiler;
        // std::unique_ptr<Profiler> mp_profiler;

        std::mutex m_globalGfxMutex;
    };

    inline auto getMaxViewportCount() -> uint32_t { return 8; }

    AP_ENUM_CLASS_OPERATORS(Device::SupportedFeatures);
    AP_ENUM_REGISTER(Device::Type);

} // namespace april::graphics
