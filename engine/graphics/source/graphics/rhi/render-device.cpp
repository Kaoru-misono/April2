// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#include "render-device.hpp"
#include "sampler.hpp"
#include "fence.hpp"
#include "compute-pipeline.hpp"
#include "graphics-pipeline.hpp"
#include "ray-tracing-pipeline.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "command-context.hpp"
#include "rhi-tools.hpp"
#include "program/program-manager.hpp"
#include "program/program.hpp"
#include "program/shader-variable.hpp"
#include "tools/blob.hpp"
#include "graphics/profile/gpu-profiler.hpp"

#include <core/error/assert.hpp>
#include <core/log/logger.hpp>
#include <core/math/math.hpp>
#include <core/tools/enum-flags.hpp>
#include <core/profile/profiler.hpp>

#include <core/profile/timer.hpp>
#include <algorithm>
#include <filesystem>
#include <ranges>

namespace april::graphics
{
    inline namespace
    {
        constexpr size_t kConstantBufferDataPlacementAlignment = 256;
        constexpr size_t kIndexBufferDataPlacementAlignment = 4;
        constexpr ShaderModel kDefaultShaderModel = ShaderModel::SM6_6;

        auto debugMessageSourceToString(rhi::DebugMessageSource source) -> std::string
        {
            switch (source)
            {
            case rhi::DebugMessageSource::Layer:  return "[Layer]";
            case rhi::DebugMessageSource::Driver: return "[Driver]";
            case rhi::DebugMessageSource::Slang:  return "[Slang]";
            default: return "[Unknown]";
            }
        }

        class GFXDebugCallBack final: public rhi::IDebugCallback
        {
            virtual SLANG_NO_THROW auto SLANG_MCALL
            handleMessage(rhi::DebugMessageType type, rhi::DebugMessageSource source, char const* message) -> void override
            {
                if (type == rhi::DebugMessageType::Error)
                {
                    AP_ERROR("{}: {}", debugMessageSourceToString(source), message);
                }
                else if (type == rhi::DebugMessageType::Warning)
                {
                    AP_WARN("{}: {}", debugMessageSourceToString(source), message);
                }
                else
                {
                    AP_TRACE("{}: {}", debugMessageSourceToString(source), message);
                }
            }
        };

        auto getGfxDeviceType(Device::Type deviceType) -> rhi::DeviceType
        {
            switch (deviceType)
            {
            case Device::Type::Default:
                return rhi::DeviceType::Default;
            case Device::Type::D3D12:
                return rhi::DeviceType::D3D12;
            case Device::Type::Vulkan:
                return rhi::DeviceType::Vulkan;
            default:
                AP_UNREACHABLE("Unknown device type");
            }
        }

        auto queryLimits(rhi::IDevice* pDevice) -> Device::Limits
        {
            auto const& deviceLimits = pDevice->getInfo().limits;

            auto toUint3 = [](uint32_t const value[]) { return uint3(value[0], value[1], value[2]); };

            Device::Limits limits = {};
            limits.maxComputeDispatchThreadGroups = toUint3(deviceLimits.maxComputeDispatchThreadGroups);
            limits.maxShaderVisibleSamplers = deviceLimits.maxShaderVisibleSamplers;
            return limits;
        }

        auto querySupportedFeatures(rhi::IDevice* pDevice) -> Device::SupportedFeatures
        {
            Device::SupportedFeatures result = Device::SupportedFeatures::None;
            if (pDevice->hasFeature("ray-tracing"))
            {
                result |= Device::SupportedFeatures::Raytracing;
            }
            if (pDevice->hasFeature("ray-query"))
            {
                result |= Device::SupportedFeatures::RaytracingTier1_1;
            }
            if (pDevice->hasFeature("conservative-rasterization-3"))
            {
                result |= Device::SupportedFeatures::ConservativeRasterizationTier3;
            }
            if (pDevice->hasFeature("conservative-rasterization-2"))
            {
                result |= Device::SupportedFeatures::ConservativeRasterizationTier2;
            }
            if (pDevice->hasFeature("conservative-rasterization-1"))
            {
                result |= Device::SupportedFeatures::ConservativeRasterizationTier1;
            }
            if (pDevice->hasFeature("rasterizer-ordered-views"))
            {
                result |= Device::SupportedFeatures::RasterizerOrderedViews;
            }

            if (pDevice->hasFeature("programmable-sample-positions-2"))
            {
                result |= Device::SupportedFeatures::ProgrammableSamplePositionsFull;
            }
            else if (pDevice->hasFeature("programmable-sample-positions-1"))
            {
                result |= Device::SupportedFeatures::ProgrammableSamplePositionsPartialOnly;
            }

            if (pDevice->hasFeature("barycentrics"))
            {
                result |= Device::SupportedFeatures::Barycentrics;
            }

            if (pDevice->hasFeature("wave-ops"))
            {
                result |= Device::SupportedFeatures::WaveOperations;
            }

            return result;
        }

        auto querySupportedShaderModel(rhi::IDevice* pDevice) -> ShaderModel
        {
            struct SMLevel
            {
                char const* name;
                ShaderModel level;
            };
            const SMLevel levels[] = {
                {"sm_6_7", ShaderModel::SM6_7},
                {"sm_6_6", ShaderModel::SM6_6},
                {"sm_6_5", ShaderModel::SM6_5},
                {"sm_6_4", ShaderModel::SM6_4},
                {"sm_6_3", ShaderModel::SM6_3},
                {"sm_6_2", ShaderModel::SM6_2},
                {"sm_6_1", ShaderModel::SM6_1},
                {"sm_6_0", ShaderModel::SM6_0},
            };
            for (auto level : levels)
            {
                if (pDevice->hasFeature(level.name))
                {
                    return level.level;
                }
            }
            return ShaderModel::Unknown;
        }

        auto getDefaultDeviceType() -> Device::Type
        {
#if defined(_WIN32)
            return Device::Type::D3D12;
#else
            return Device::Type::Vulkan;
#endif
        }

        // Copy from slang-rhi test.
        struct PersistentCache: rhi::IPersistentCache
        {
        public:
            struct Stats
            {
                uint32_t missCount{};
                uint32_t hitCount{};
                uint32_t entryCount{};
            };

            using Key = std::vector<uint8_t>;
            using Data = std::vector<uint8_t>;

            struct Entry
            {
                uint32_t ticket{};
                std::vector<uint8_t> data{};
            };

            std::map<Key, Entry> entries{};
            Stats stats{};
            uint32_t maxEntryCount{1024};
            uint32_t ticketCounter{0};

            virtual ~PersistentCache() = default;

            void clear()
            {
                entries.clear();
                stats = {};
                maxEntryCount = 1024;
                ticketCounter = 0;
            }

            SLANG_NO_THROW rhi::Result SLANG_MCALL writeCache(ISlangBlob* key_, ISlangBlob* data_) override
            {
                while (entries.size() >= maxEntryCount)
                {
                    auto it = std::min_element(
                        entries.begin(),
                        entries.end(),
                        [](auto const& a, auto const& b) { return a.second.ticket < b.second.ticket; }
                    );
                    entries.erase(it);
                }

                Key key(
                    static_cast<uint8_t const*>(key_->getBufferPointer()),
                    static_cast<uint8_t const*>(key_->getBufferPointer()) + key_->getBufferSize()
                );
                Data data(
                    static_cast<uint8_t const*>(data_->getBufferPointer()),
                    static_cast<uint8_t const*>(data_->getBufferPointer()) + data_->getBufferSize()
                );
                entries[key] = {ticketCounter++, data};
                stats.entryCount = (uint32_t) entries.size();
                return SLANG_OK;
            }

            SLANG_NO_THROW rhi::Result SLANG_MCALL queryCache(ISlangBlob* key_, ISlangBlob** outData) override
            {
                Key key(
                    static_cast<uint8_t const*>(key_->getBufferPointer()),
                    static_cast<uint8_t const*>(key_->getBufferPointer()) + key_->getBufferSize()
                );
                auto it = entries.find(key);
                if (it == entries.end())
                {
                    stats.missCount++;
                    *outData = nullptr;
                    return SLANG_E_NOT_FOUND;
                }
                stats.hitCount++;
                *outData = SimpleBlob::create(it->second.data.data(), it->second.data.size()).detach();
                return SLANG_OK;
            }

            virtual SLANG_NO_THROW rhi::Result SLANG_MCALL queryInterface(SlangUUID const& uuid, void** outObject) override
            {
                if (uuid == IPersistentCache::getTypeGuid())
                {
                    *outObject = static_cast<IPersistentCache*>(this);
                    return SLANG_OK;
                }
                return SLANG_E_NO_INTERFACE;
            }

            virtual SLANG_NO_THROW uint32_t SLANG_MCALL addRef() override
            {
                return 2;
            }

            virtual SLANG_NO_THROW uint32_t SLANG_MCALL release() override
            {
                return 2;
            }
        };
    } // namespace

    Device::Device(Desc const& desc) : m_desc(desc)
    {
        checkResult(slang::createGlobalSession(m_slangGlobalSession.writeRef()), "Failed to create Slang global session");
        // ... (skipping some lines for context match if needed, but I'll try exact)

        if (m_desc.type == Type::Default)
        {
            m_desc.type = getDefaultDeviceType();
        }

        m_callback = std::make_unique<GFXDebugCallBack>();
        m_shaderCache = std::make_unique<PersistentCache>();
        m_pipelineCache = std::make_unique<PersistentCache>();

        auto RHI = rhi::getRHI();

        rhi::DeviceDesc gfxDesc = {};
        gfxDesc.deviceType = getGfxDeviceType(m_desc.type);
        gfxDesc.slang.slangGlobalSession = m_slangGlobalSession;
        gfxDesc.debugCallback = m_callback.get();
        auto shaderSearchPath = (std::filesystem::current_path() / "shader/graphics").string();
        char const* paths[] = { shaderSearchPath.c_str() };
        gfxDesc.slang.searchPathCount = 1;
        gfxDesc.slang.searchPaths = paths;
        gfxDesc.slang.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
        // Setup shader cache
        gfxDesc.persistentShaderCache = m_shaderCache.get();
        gfxDesc.persistentPipelineCache = m_pipelineCache.get();

        gfxDesc.enableValidation = m_desc.enableDebugLayer;

        auto gpus = getGPUs(m_desc.type);
        if (gpus.empty())
        {
            AP_CRITICAL("Did not find any GPUs for device type");
        }

        if (m_desc.gpu >= gpus.size())
        {
            AP_WARN("GPU index out of range, using first GPU");
            m_desc.gpu = 0;
        }

        checkResult(RHI->createDevice(gfxDesc, m_gfxDevice.writeRef()), "Failed to create device");

        // gfxDesc.adapterLUID = reinterpret_cast<rhi::AdapterLUID const*>(&gpus[m_desc.gpu].luid);
        // if (SLANG_FAILED(rhi::gfxCreateDevice(&gfxDesc, m_gfxDevice.writeRef())))
        // {
        //     gfxDesc.adapterLUID = nullptr;
        //     checkResult(rhi::gfxCreateDevice(&gfxDesc, m_gfxDevice.writeRef()), "Failed to create device");
        // }

        auto const& deviceInfo = m_gfxDevice->getInfo();
        m_info.adapterName = deviceInfo.adapterName;
        m_info.adapterLUID = gpus[m_desc.gpu].luid;
        m_info.apiName = deviceInfo.apiName;
        m_limits = queryLimits(m_gfxDevice);
        m_supportedFeatures = querySupportedFeatures(m_gfxDevice);

        m_supportedShaderModel = querySupportedShaderModel(m_gfxDevice);
        m_defaultShaderModel = std::min(kDefaultShaderModel, m_supportedShaderModel);
        m_gpuTimestampFrequency = deviceInfo.timestampFrequency;

        checkResult(m_gfxDevice->getQueue(rhi::QueueType::Graphics, m_gfxCommandQueue.writeRef()), "Failed to get command queue");

        this->incRef();

        mp_frameFence = createFence();
        mp_frameFence->breakStrongReferenceToDevice();

        mp_programManager = std::make_unique<ProgramManager>(this);
        // mp_profiler = std::make_unique<Profiler>(core::ref<Device>(this));

        mp_defaultSampler = createSampler(Sampler::Desc());
        mp_defaultSampler->breakStrongReferenceToDevice();

        mp_uploadHeap = GpuMemoryHeap::create(core::ref<Device>(this), MemoryType::Upload, 1024 * 1024 * 2, mp_frameFence);
        mp_uploadHeap->breakStrongReferenceToDevice();

        mp_readBackHeap = GpuMemoryHeap::create(core::ref<Device>(this), MemoryType::ReadBack, 1024 * 1024 * 2, mp_frameFence);
        mp_readBackHeap->breakStrongReferenceToDevice();

        mp_timestampQueryHeap = QueryHeap::create(core::ref<Device>(this), QueryHeap::Type::Timestamp, 1024 * 1024);
        mp_timestampQueryHeap->breakStrongReferenceToDevice();

        mp_commandContext = std::unique_ptr<CommandContext>(new CommandContext(this, m_gfxCommandQueue));

        mp_gpuProfiler = GpuProfiler::create(core::ref<Device>(this));
        core::Profiler::get().registerGpuProfiler(mp_gpuProfiler.get());

        // TODO: Do we need to flush here?
        mp_commandContext->submit();
        mp_gpuProfiler->beginFrameCalibration(mp_commandContext.get());

        this->decRef(false);

        AP_INFO("Created GPU device '{}' using '{}' API.", m_info.adapterName, m_info.apiName);
    }

    Device::~Device()
    {
        mp_commandContext->submit(true);

        // Force flush all deferred releases
        mp_uploadHeap->executeDeferredReleases();
        mp_readBackHeap->executeDeferredReleases();
        while (!m_deferredReleases.empty())
        {
            m_deferredReleases.pop();
        }

        m_gfxCommandQueue.setNull();
        mp_commandContext.reset();
        mp_uploadHeap.reset();
        mp_readBackHeap.reset();
        mp_timestampQueryHeap.reset();

        mp_defaultSampler.reset();
        mp_frameFence.reset();
        mp_programManager.reset();
        m_gfxDevice.setNull();
    }

    auto Device::createBuffer(size_t size, BufferUsage usage, MemoryType memoryType, void const* pInitData) -> core::ref<Buffer>
    {
        return core::make_ref<Buffer>(core::ref<Device>(this), size, usage, memoryType, pInitData);
    }

    auto Device::createTypedBuffer(
        ResourceFormat format,
        uint32_t elementCount,
        BufferUsage usage,
        MemoryType memoryType,
        void const* pInitData
    ) -> core::ref<Buffer>
    {
        return core::make_ref<Buffer>(core::ref<Device>(this), format, elementCount, usage, memoryType, pInitData);
    }

    auto Device::createStructuredBuffer(
        uint32_t structSize,
        uint32_t elementCount,
        BufferUsage usage,
        MemoryType memoryType,
        void const* pInitData,
        bool createCounter
    ) -> core::ref<Buffer>
    {
        return core::make_ref<Buffer>(core::ref<Device>(this), structSize, elementCount, usage, memoryType, pInitData, createCounter);
    }

    auto Device::createStructuredBuffer(
        ReflectionType const* pType,
        uint32_t elementCount,
        BufferUsage usage,
        MemoryType memoryType,
        void const* pInitData,
        bool createCounter
    ) -> core::ref<Buffer>
    {
        AP_ASSERT(pType != nullptr, "Can't create a structured buffer from a nullptr type.");
        auto const* pResourceType = pType->unwrapArray()->asResourceType();
        if (!pResourceType || pResourceType->getType() != ReflectionResourceType::Type::StructuredBuffer)
        {
            AP_ERROR("Can't create a structured buffer from type '{}'.", pType->getClassName());
            return nullptr;
        }

        auto structStride = pResourceType->getStructType()->getSlangTypeLayout()->getStride();
        return core::make_ref<Buffer>(core::ref<Device>(this), (uint32_t)structStride, elementCount, usage, memoryType, pInitData, createCounter);
    }

    auto Device::createStructuredBuffer(
        ShaderVariable const& shaderVariable,
        uint32_t elementCount,
        BufferUsage usage,
        MemoryType memoryType,
        void const* pInitData,
        bool createCounter
    ) -> core::ref<Buffer>
    {
        return createStructuredBuffer(shaderVariable.getType(), elementCount, usage, memoryType, pInitData, createCounter);
    }

    auto Device::createBufferFromResource(rhi::IBuffer* pResource, size_t size, BufferUsage usage, MemoryType memoryType) -> core::ref<Buffer>
    {
        return core::make_ref<Buffer>(core::ref<Device>(this), pResource, size, usage, memoryType);
    }

    auto Device::createBufferFromNativeHandle(NativeHandle handle, size_t size, BufferUsage usage, MemoryType memoryType) -> core::ref<Buffer>
    {
        return core::make_ref<Buffer>(core::ref<Device>(this), handle, size, usage, memoryType);
    }

    auto Device::createTexture1D(
        uint32_t width,
        ResourceFormat format,
        uint32_t arraySize,
        uint32_t mipLevels,
        void const* pInitData,
        TextureUsage usage
    ) -> core::ref<Texture>
    {
        return core::make_ref<Texture>(core::ref<Device>(this), Resource::Type::Texture1D, format, width, 1, 1, arraySize, mipLevels, 1, usage, pInitData);
    }

    auto Device::createTexture2D(
        uint32_t width,
        uint32_t height,
        ResourceFormat format,
        uint32_t arraySize,
        uint32_t mipLevels,
        void const* pInitData,
        TextureUsage usage
    ) -> core::ref<Texture>
    {
        return core::make_ref<Texture>(core::ref<Device>(this), Resource::Type::Texture2D, format, width, height, 1, arraySize, mipLevels, 1, usage, pInitData);
    }

    auto Device::createTexture3D(
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        ResourceFormat format,
        uint32_t mipLevels,
        void const* pInitData,
        TextureUsage usage
    ) -> core::ref<Texture>
    {
        return core::make_ref<Texture>(core::ref<Device>(this), Resource::Type::Texture3D, format, width, height, depth, 1, mipLevels, 1, usage, pInitData);
    }

    auto Device::createTextureCube(
        uint32_t width,
        uint32_t height,
        ResourceFormat format,
        uint32_t arraySize,
        uint32_t mipLevels,
        void const* pInitData,
        TextureUsage usage
    ) -> core::ref<Texture>
    {
        return core::make_ref<Texture>(core::ref<Device>(this), Resource::Type::TextureCube, format, width, height, 1, arraySize, mipLevels, 1, usage, pInitData);
    }

    auto Device::createTexture2DMS(
        uint32_t width,
        uint32_t height,
        ResourceFormat format,
        uint32_t sampleCount,
        uint32_t arraySize,
        TextureUsage usage
    ) -> core::ref<Texture>
    {
        return core::make_ref<Texture>(core::ref<Device>(this), Resource::Type::Texture2DMS, format, width, height, 1, arraySize, 1, sampleCount, usage, nullptr);
    }

    auto Device::createTextureFromResource(
        rhi::ITexture* pResource,
        Resource::Type type,
        ResourceFormat format,
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        uint32_t arraySize,
        uint32_t mipLevels,
        uint32_t sampleCount,
        TextureUsage usage,
        Resource::State initState
    ) -> core::ref<Texture>
    {
        return core::make_ref<Texture>(core::ref<Device>(this), pResource, type, format, width, height, depth, arraySize, mipLevels, sampleCount, usage, initState);
    }

    inline namespace
    {
        auto convertAssetFormat(asset::PixelFormat format) -> ResourceFormat
        {
            switch (format)
            {
            case asset::PixelFormat::R8Unorm:
                return ResourceFormat::R8Unorm;
            case asset::PixelFormat::RG8Unorm:
                return ResourceFormat::RG8Unorm;
            case asset::PixelFormat::RGBA8Unorm:
                return ResourceFormat::RGBA8Unorm;
            case asset::PixelFormat::RGBA8UnormSrgb:
                return ResourceFormat::RGBA8UnormSrgb;
            default:
                return ResourceFormat::Unknown;
            }
        }
    } // namespace

    auto Device::createTextureFromAsset(
        asset::AssetManager& assetManager,
        asset::TextureAsset const& asset,
        TextureUsage usage,
        bool generateMips
    ) -> core::ref<Texture>
    {
        // Get compiled texture data from asset manager
        auto blob = std::vector<std::byte>{};
        auto payload = assetManager.getTextureData(asset, blob);

        if (!payload.isValid())
        {
            AP_ERROR("[Device] Failed to get texture data for asset: {}", asset.getSourcePath());
            return nullptr;
        }

        auto const& header = payload.header;

        // Convert asset format to graphics format
        auto format = convertAssetFormat(header.format);
        if (format == ResourceFormat::Unknown)
        {
            AP_ERROR("[Device] Unknown texture format for asset: {}", asset.getSourcePath());
            return nullptr;
        }

        // Determine mip levels (note: texture init data currently uploads only one subresource).
        auto mipLevels = 1u;
        if (generateMips && header.mipLevels > 1)
        {
            AP_WARN("[Device] Mip upload not supported yet; using base mip only for {}", asset.getSourcePath());
        }

        // Create the texture with initial data
        auto texture = createTexture2D(
            header.width,
            header.height,
            format,
            1, // arraySize
            mipLevels,
            payload.pixelData.data(),
            usage
        );

        if (texture)
        {
            texture->setSourcePath(asset.getSourcePath());

            AP_INFO("[Device] Created texture from asset: {}x{} {} ({})",
                    header.width, header.height, to_string(format), asset.getSourcePath());
        }

        return texture;
    }

    auto Device::createMeshFromAsset(
        asset::AssetManager& assetManager,
        asset::StaticMeshAsset const& asset
    ) -> core::ref<StaticMesh>
    {
        // Get compiled mesh data from asset manager
        auto blob = std::vector<std::byte>{};
        auto payload = assetManager.getMeshData(asset, blob);

        if (!payload.isValid())
        {
            AP_ERROR("[Device] Failed to get mesh data for asset: {}", asset.getSourcePath());
            return nullptr;
        }

        auto const& header = payload.header;

        // Create vertex layout
        // Standard format: position(3) + normal(3) + tangent(4) + texcoord(2) = 12 floats = 48 bytes
        auto bufferLayout = VertexBufferLayout::create();
        bufferLayout->addElement("POSITION", 0, ResourceFormat::RGB32Float, 1, 0);
        bufferLayout->addElement("NORMAL", 12, ResourceFormat::RGB32Float, 1, 1);
        bufferLayout->addElement("TANGENT", 24, ResourceFormat::RGBA32Float, 1, 2);
        bufferLayout->addElement("TEXCOORD", 40, ResourceFormat::RG32Float, 1, 3);

        auto vertexLayout = VertexLayout::create();
        vertexLayout->addBufferLayout(0, bufferLayout);

        // Create vertex buffer
        auto vertexBuffer = createBuffer(
            header.vertexDataSize,
            BufferUsage::VertexBuffer | BufferUsage::ShaderResource,
            MemoryType::DeviceLocal,
            payload.vertexData.data()
        );

        if (!vertexBuffer)
        {
            AP_ERROR("[Device] Failed to create vertex buffer for asset: {}", asset.getSourcePath());
            return nullptr;
        }

        // Create index buffer
        auto indexFormat = header.indexFormat == 0 ? ResourceFormat::R16Uint : ResourceFormat::R32Uint;
        auto indexBuffer = createBuffer(
            header.indexDataSize,
            BufferUsage::IndexBuffer | BufferUsage::ShaderResource,
            MemoryType::DeviceLocal,
            payload.indexData.data()
        );

        if (!indexBuffer)
        {
            AP_ERROR("[Device] Failed to create index buffer for asset: {}", asset.getSourcePath());
            return nullptr;
        }

        // Create VAO
        auto vao = VertexArrayObject::create(
            VertexArrayObject::Topology::TriangleList,
            vertexLayout,
            {vertexBuffer},
            indexBuffer,
            indexFormat
        );

        if (!vao)
        {
            AP_ERROR("[Device] Failed to create VAO for asset: {}", asset.getSourcePath());
            return nullptr;
        }

        // Convert submeshes to draw ranges
        auto drawRanges = std::vector<StaticMesh::DrawRange>{};
        drawRanges.reserve(payload.submeshes.size());
        for (auto const& submesh : payload.submeshes)
        {
            drawRanges.push_back({
                submesh.indexOffset,
                submesh.indexCount,
                submesh.materialIndex
            });
        }

        // Create StaticMesh
        auto mesh = core::make_ref<StaticMesh>(
            vao,
            std::move(drawRanges),
            std::array<float, 3>{header.boundsMin[0], header.boundsMin[1], header.boundsMin[2]},
            std::array<float, 3>{header.boundsMax[0], header.boundsMax[1], header.boundsMax[2]}
        );

        AP_INFO("[Device] Created mesh from asset: {} vertices, {} indices, {} submeshes ({})",
                header.vertexCount, header.indexCount, header.submeshCount, asset.getSourcePath());

        return mesh;
    }

    auto Device::createHeap(rhi::HeapDesc const& desc) -> Slang::ComPtr<rhi::IHeap>
    {
        Slang::ComPtr<rhi::IHeap> p_heap;
        checkResult(m_gfxDevice->createHeap(desc, p_heap.writeRef()), "Failed to create heap");
        return p_heap;
    }

    auto Device::createSampler(Sampler::Desc const& desc) -> core::ref<Sampler>
    {
        return core::make_ref<Sampler>(core::ref<Device>(this), desc);
    }

    auto Device::createFence(FenceDesc const& desc) -> core::ref<Fence>
    {
        return core::make_ref<Fence>(core::ref<Device>(this), desc);
    }

    auto Device::createFence(bool shared) -> core::ref<Fence>
    {
        FenceDesc desc;
        desc.shared = shared;
        return createFence(desc);
    }

    auto Device::createComputePipeline(ComputePipelineDesc const& desc) -> core::ref<ComputePipeline>
    {
        return core::make_ref<ComputePipeline>(core::ref<Device>(this), desc);
    }

    auto Device::createGraphicsPipeline(GraphicsPipelineDesc const& desc) -> core::ref<GraphicsPipeline>
    {
        return core::make_ref<GraphicsPipeline>(core::ref<Device>(this), desc);
    }

    auto Device::createRayTracingPipeline(RayTracingPipelineDesc const& desc) -> core::ref<RayTracingPipeline>
    {
        return core::make_ref<RayTracingPipeline>(core::ref<Device>(this), desc);
    }

    auto Device::getBlitPipeline(ResourceFormat format, uint32_t sampleCount) -> core::ref<GraphicsPipeline>
    {
        std::scoped_lock lock(m_blitMutex);

        auto key = std::make_pair(format, sampleCount);
        auto it = m_blitPipelines.find(key);
        if (it != m_blitPipelines.end())
        {
            return it->second;
        }

        if (!mp_blitProgram)
        {
            ProgramDesc progDesc;
            progDesc.addShaderLibrary("graphics/blit.slang");
            progDesc.vsEntryPoint("vertexMain");
            progDesc.psEntryPoint("fragmentMain");
            mp_blitProgram = Program::create(core::ref<Device>(this), progDesc);
            mp_blitProgram->breakStrongReferenceToDevice();
        }

        GraphicsPipelineDesc pipelineDesc;
        pipelineDesc.programKernels = mp_blitProgram->getActiveVersion()->getKernels(this, nullptr);
        pipelineDesc.renderTargetCount = 1;
        pipelineDesc.renderTargetFormats[0] = format;
        pipelineDesc.sampleCount = sampleCount;
        pipelineDesc.primitiveType = GraphicsPipelineDesc::PrimitiveType::TriangleList;

        RasterizerState::Desc rsDesc;
        rsDesc.setCullMode(RasterizerState::CullMode::None);
        rsDesc.setScissorTest(true);
        pipelineDesc.rasterizerState = RasterizerState::create(rsDesc);

        DepthStencilState::Desc dsDesc;
        dsDesc.setDepthEnabled(false);
        dsDesc.setDepthWriteMask(false);
        dsDesc.setStencilEnabled(false);
        pipelineDesc.depthStencilState = DepthStencilState::create(dsDesc);

        auto pipeline = createGraphicsPipeline(pipelineDesc);
        m_blitPipelines.emplace(key, pipeline);
        return pipeline;
    }

    auto Device::getBlitSampler(TextureFilteringMode filter) -> core::ref<Sampler>
    {
        std::scoped_lock lock(m_blitMutex);

        auto ensureSampler = [this](TextureFilteringMode mode, core::ref<Sampler>& slot) {
            if (slot) return;
            Sampler::Desc desc;
            desc.setFilterMode(mode, mode, mode);
            desc.setAddressingMode(TextureAddressingMode::Clamp, TextureAddressingMode::Clamp, TextureAddressingMode::Clamp);
            slot = createSampler(desc);
        };

        if (filter == TextureFilteringMode::Point)
        {
            ensureSampler(TextureFilteringMode::Point, mp_blitPointSampler);
            return mp_blitPointSampler;
        }

        ensureSampler(TextureFilteringMode::Linear, mp_blitLinearSampler);
        return mp_blitLinearSampler;
    }

    auto Device::getProgramManager() const -> ProgramManager*
    {
        return mp_programManager.get();
    }

    auto Device::getProfiler() const -> core::Profiler*
    {
        return nullptr; // mp_profiler.get();
    }

    auto Device::getGpuProfiler() const -> GpuProfiler*
    {
        return mp_gpuProfiler.get();
    }

    auto Device::getShaderCacheStats() const -> CacheStats
    {
        auto cache = static_cast<PersistentCache*>(m_shaderCache.get());
        return {cache->stats.hitCount, cache->stats.missCount, cache->stats.entryCount};
    }

    auto Device::getPipelineCacheStats() const -> CacheStats
    {
        auto cache = static_cast<PersistentCache*>(m_pipelineCache.get());
        return {cache->stats.hitCount, cache->stats.missCount, cache->stats.entryCount};
    }

    auto Device::getCommandContext() const -> CommandContext*
    {
        return mp_commandContext.get();
    }

    auto Device::endFrame() -> void
    {
        if (mp_gpuProfiler)
        {
            mp_gpuProfiler->endFrame(mp_commandContext.get());
        }

        auto payload = mp_commandContext->finish();

        uint64_t nextFenceValue = mp_frameFence->getSignaledValue() + 1;
        mp_frameFence->updateSignaledValue(nextFenceValue);

        auto waitFences = payload.waitFences | std::views::keys | std::ranges::to<std::vector>();
        auto waitValues = payload.waitFences | std::views::values | std::ranges::to<std::vector>();

        auto signalFences = payload.signalFences | std::views::keys | std::ranges::to<std::vector>();
        auto signalValues = payload.signalFences | std::views::values | std::ranges::to<std::vector>();

        signalFences.emplace_back(mp_frameFence->getGfxFence());
        signalValues.emplace_back(nextFenceValue);

        auto commandBuffer = payload.commandBuffer.get();

        rhi::SubmitDesc submitDesc = {};
        submitDesc.commandBuffers = payload.commandBuffer ? &commandBuffer : nullptr;
        submitDesc.commandBufferCount = payload.commandBuffer ? 1 : 0;

        submitDesc.waitFences = waitFences.data();
        submitDesc.waitFenceValues = waitValues.data();
        submitDesc.waitFenceCount = (uint32_t)waitFences.size();

        submitDesc.signalFences = signalFences.data();
        submitDesc.signalFenceValues = signalValues.data();
        submitDesc.signalFenceCount = (uint32_t)signalFences.size();

        auto cpuStart = core::Timer::now();

        checkResult(m_gfxCommandQueue->submit(submitDesc), "Failed to submit frame command buffer");

        auto cpuEnd = core::Timer::now();

        if (mp_gpuProfiler)
        {
            auto avg = cpuStart + (cpuEnd - cpuStart) / 2;

            mp_gpuProfiler->postSubmit(
                mp_commandContext.get(),
                std::chrono::duration_cast<std::chrono::nanoseconds>(avg.time_since_epoch()).count(),
                nextFenceValue
            );
        }

        executeDeferredReleases();

        if (m_desc.enableRaytracingValidation)
        {
            flushRaytracingValidation();
        }
    }

    auto Device::wait() -> void
    {
        // Flush any pending work, then block until the queue is idle.
        // CommandContext::submit(true) can early-out when no work is pending,
        // so call waitOnHost() explicitly to ensure idle even in that case.
        mp_commandContext->submit(false);
        if (m_gfxCommandQueue)
        {
            m_gfxCommandQueue->waitOnHost();
        }
        executeDeferredReleases();
    }

    auto Device::requireD3D12() const -> void
    {
        AP_ASSERT(m_desc.type == Type::D3D12);
    }

    auto Device::requireVulkan() const -> void
    {
        AP_ASSERT(m_desc.type == Type::Vulkan);
    }

    auto Device::getBufferDataAlignment(BufferUsage usage) -> size_t
    {
        if (enum_has_any_flags(usage, BufferUsage::ConstantBuffer))
        {
            return kConstantBufferDataPlacementAlignment;
        }
        if (enum_has_any_flags(usage, BufferUsage::IndexBuffer))
        {
            return kIndexBufferDataPlacementAlignment;
        }
        return 1;
    }

    auto Device::releaseResource(ISlangUnknown* pResource) -> void
    {
        if (pResource)
        {
            m_deferredReleases.push({mp_frameFence ? mp_frameFence->getSignaledValue() : 0, Slang::ComPtr<ISlangUnknown>(pResource)});
        }
    }

    auto Device::isFeatureSupported(SupportedFeatures flags) const -> bool
    {
        return enum_has_any_flags(m_supportedFeatures, flags);
    }

    auto Device::isShaderModelSupported(ShaderModel shaderModel) const -> bool
    {
        return (uint32_t)shaderModel <= (uint32_t)m_supportedShaderModel;
    }

    auto Device::executeDeferredReleases() -> void
    {
        mp_uploadHeap->executeDeferredReleases();
        mp_readBackHeap->executeDeferredReleases();
        uint64_t currentValue = mp_frameFence->getCurrentValue();
        while (!m_deferredReleases.empty() && m_deferredReleases.front().fenceValue <= currentValue)
        {
            m_deferredReleases.pop();
        }
    }

    auto Device::getTextureRowAlignment() const -> size_t
    {
        size_t alignment = 1;
        m_gfxDevice->getTextureRowAlignment(&alignment);
        return alignment;
    }

    auto Device::getNativeHandle(uint32_t index) const -> rhi::NativeHandle
    {
        rhi::DeviceNativeHandles gfxInteropHandles = {};
        checkResult(m_gfxDevice->getNativeDeviceHandles(&gfxInteropHandles), "Failed to get native device handles");
        if (index < 3)
        {
            return gfxInteropHandles.handles[index];
        }
        return {};
    }

    auto Device::reportLiveObjects() -> void
    {

    }

    auto Device::enableAgilitySDK() -> bool
    {
        return false;
    }

    auto Device::getGPUs(Type deviceType) -> std::vector<AdapterInfo>
    {
        if (deviceType == Type::Default)
        {
            deviceType = getDefaultDeviceType();
        }
        auto RHI = rhi::getRHI();
        auto adapters = RHI->getAdapters(getGfxDeviceType(deviceType));
        std::vector<AdapterInfo> result;
        for (uint32_t i = 0; i < adapters.getCount(); ++i)
        {
            auto const& gfxInfo = adapters.getAdapters()[i];
            AdapterInfo info;
            info.name = gfxInfo.name;
            info.vendorID = gfxInfo.vendorID;
            info.deviceID = gfxInfo.deviceID;
            info.luid = *reinterpret_cast<AdapterLUID const*>(&gfxInfo.luid);
            result.push_back(info);
        }
        return result;
    }

    auto Device::flushRaytracingValidation() -> void {}

    auto Device::enableRaytracingValidation() -> void
    {
        // TODO
    }

    auto Device::disableRaytracingValidation() -> void
    {
    }

} // namespace april::graphics
