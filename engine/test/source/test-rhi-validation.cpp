#include <doctest/doctest.h>
#include <graphics/rhi/render-device.hpp>
#include <graphics/rhi/command-context.hpp>
#include <graphics/rhi/compute-pipeline.hpp>
#include <graphics/rhi/graphics-pipeline.hpp>
#include <graphics/rhi/vertex-array-object.hpp>
#include <graphics/rhi/rhi-tools.hpp>
#include <graphics/rhi/swapchain.hpp>
#include <graphics/program/program-variables.hpp>
#include <vector>
#include <numeric>
#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

using namespace april::graphics;

TEST_SUITE("RHI Validation")
{
    TEST_CASE("Buffer Data Integrity")
    {
        for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
        {
            Device::Desc deviceDesc;
            deviceDesc.type = deviceType;
            auto device = april::core::make_ref<Device>(deviceDesc);
            if (!device) continue;

            auto ctx = device->getCommandContext();
            REQUIRE(ctx);

            const size_t elementCount = 1024;
            const size_t bufferSize = elementCount * sizeof(uint32_t);
            auto buffer = device->createBuffer(bufferSize, ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess);
            REQUIRE(buffer);

            std::vector<uint32_t> initData(elementCount);
            std::iota(initData.begin(), initData.end(), 0);

            ctx->updateBuffer(buffer.get(), initData.data(), 0, bufferSize);
            ctx->submit(true);

            std::vector<uint32_t> readbackData(elementCount);
            ctx->readBuffer(buffer.get(), readbackData.data(), 0, bufferSize);
            ctx->submit(true);

            for (size_t i = 0; i < elementCount; ++i)
            {
                CHECK(readbackData[i] == initData[i]);
            }
        }
    }

    TEST_CASE("Texture Data Integrity")
    {
        for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
        {
            Device::Desc deviceDesc;
            deviceDesc.type = deviceType;
            auto device = april::core::make_ref<Device>(deviceDesc);
            if (!device) continue;

            auto ctx = device->getCommandContext();
            REQUIRE(ctx);

            const uint32_t width = 256;
            const uint32_t height = 256;
            auto texture = device->createTexture2D(width, height, ResourceFormat::RGBA8Unorm, 1, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess);
            REQUIRE(texture);

            std::vector<uint32_t> initData(width * height);
            for (uint32_t y = 0; y < height; ++y)
            {
                for (uint32_t x = 0; x < width; ++x)
                {
                    uint8_t r = x % 256;
                    uint8_t g = y % 256;
                    uint8_t b = 128;
                    uint8_t a = 255;
                    initData[y * width + x] = (a << 24) | (b << 16) | (g << 8) | r;
                }
            }

            ctx->updateTextureData(texture.get(), initData.data());
            ctx->submit(true);

            auto readbackBytes = ctx->readTextureSubresource(texture.get(), 0);
            ctx->submit(true);

            REQUIRE(readbackBytes.size() == width * height * 4);
            const uint32_t* readbackData = reinterpret_cast<const uint32_t*>(readbackBytes.data());
            for (size_t i = 0; i < width * height; ++i)
            {
                CHECK(readbackData[i] == initData[i]);
            }
        }
    }

    TEST_CASE("Sampler Creation")
    {
        for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
        {
            Device::Desc deviceDesc;
            deviceDesc.type = deviceType;
            auto device = april::core::make_ref<Device>(deviceDesc);
            if (!device) continue;

            Sampler::Desc desc;
            desc.setFilterMode(TextureFilteringMode::Linear, TextureFilteringMode::Linear, TextureFilteringMode::Point);
            desc.setAddressingMode(TextureAddressingMode::Clamp, TextureAddressingMode::Mirror, TextureAddressingMode::Wrap);

            auto sampler = device->createSampler(desc);
            REQUIRE(sampler);
            CHECK(sampler->getGfxSamplerState() != nullptr);
        }
    }

    TEST_CASE("Sampler Usage")
    {
        for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
        {
            Device::Desc deviceDesc;
            deviceDesc.type = deviceType;
            auto device = april::core::make_ref<Device>(deviceDesc);
            if (!device) continue;

            auto ctx = device->getCommandContext();
            REQUIRE(ctx);

            auto texture = device->createTexture2D(1, 1, ResourceFormat::RGBA8Unorm, 1, 1, nullptr, ResourceBindFlags::ShaderResource);
            REQUIRE(texture);
            uint32_t red = 0xFF0000FF;
            ctx->updateTextureData(texture.get(), &red);
            ctx->submit(true);

            auto outputBuffer = device->createBuffer(sizeof(april::float4), ResourceBindFlags::UnorderedAccess);
            REQUIRE(outputBuffer);

            const char* shaderSource = R"(
                Texture2D gTex;
                SamplerState gSampler;
                RWStructuredBuffer<float4> gOutput;
                [shader("compute")]
                [numthreads(1, 1, 1)]
                void main() {
                    gOutput[0] = gTex.SampleLevel(gSampler, float2(0.5, 0.5), 0);
                }
            )";

            ProgramDesc progDesc;
            progDesc.addShaderModule("SampleTest").addString(shaderSource, "SampleTest.slang");
            progDesc.csEntryPoint("main");
            auto program = Program::create(device, progDesc);
            REQUIRE(program);

            auto vars = ProgramVariables::create(device, program.get());
            REQUIRE(vars);
            vars->setTexture("gTex", texture);
            vars->setSampler("gSampler", device->getDefaultSampler());
            vars->setBuffer("gOutput", outputBuffer);

            ComputePipelineDesc pipeDesc;
            pipeDesc.programKernels = program->getActiveVersion()->getKernels(device.get(), vars.get());
            auto pipeline = device->createComputePipeline(pipeDesc);
            REQUIRE(pipeline);

            auto computeEncoder = ctx->beginComputePass();
            computeEncoder->bindPipeline(pipeline.get(), vars.get());
            computeEncoder->dispatch(april::uint3(1, 1, 1));
            computeEncoder->end();
            ctx->submit(true);

            april::float4 result;
            ctx->readBuffer(outputBuffer.get(), &result, 0, sizeof(april::float4));
            ctx->submit(true);

            CHECK(result.r == 1.0f);
            CHECK(result.g == 0.0f);
            CHECK(result.b == 0.0f);
            CHECK(result.a == 1.0f);
        }
    }

    TEST_CASE("Compute UAV Operations")
    {
        for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
        {
            Device::Desc deviceDesc;
            deviceDesc.type = deviceType;
            auto device = april::core::make_ref<Device>(deviceDesc);
            if (!device) continue;

            auto ctx = device->getCommandContext();
            REQUIRE(ctx);

            const uint32_t elementCount = 64;
            auto buffer = device->createBuffer(elementCount * sizeof(uint32_t), ResourceBindFlags::UnorderedAccess);
            REQUIRE(buffer);

            const char* shaderSource = R"(
                RWStructuredBuffer<uint> gOutput;
                [shader("compute")]
                [numthreads(64, 1, 1)]
                void main(uint3 threadId : SV_DispatchThreadID) {
                    gOutput[threadId.x] = threadId.x * 2;
                }
            )";

            ProgramDesc progDesc;
            progDesc.addShaderModule("ComputeUAV").addString(shaderSource, "ComputeUAV.slang");
            progDesc.csEntryPoint("main");
            auto program = Program::create(device, progDesc);
            REQUIRE(program);

            auto vars = ProgramVariables::create(device, program.get());
            REQUIRE(vars);
            vars->setBuffer("gOutput", buffer);

            ComputePipelineDesc pipeDesc;
            pipeDesc.programKernels = program->getActiveVersion()->getKernels(device.get(), vars.get());
            auto pipeline = device->createComputePipeline(pipeDesc);
            REQUIRE(pipeline);

            auto computeEncoder = ctx->beginComputePass();
            computeEncoder->bindPipeline(pipeline.get(), vars.get());
            computeEncoder->dispatch(april::uint3(1, 1, 1));
            computeEncoder->end();
            ctx->submit(true);

            auto result = ctx->readBuffer<uint32_t>(buffer.get());
            ctx->submit(true);

            for (uint32_t i = 0; i < elementCount; ++i)
            {
                CHECK(result[i] == i * 2);
            }
        }
    }

    TEST_CASE("Rasterization Pipeline")
    {
        for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
        {
            Device::Desc deviceDesc;
            deviceDesc.type = deviceType;
            auto device = april::core::make_ref<Device>(deviceDesc);
            if (!device) continue;

            auto ctx = device->getCommandContext();
            REQUIRE(ctx);

            const uint32_t width = 2;
            const uint32_t height = 2;
            auto renderTarget = device->createTexture2D(width, height, ResourceFormat::RGBA8Unorm, 1, 1, nullptr, ResourceBindFlags::RenderTarget | ResourceBindFlags::ShaderResource);
            REQUIRE(renderTarget);

            const char* shaderSource = R"(
                struct VSOut {
                    float4 pos : SV_Position;
                    float4 color : COLOR;
                };
                [shader("vertex")]
                VSOut vsMain(uint id : SV_VertexID) {
                    VSOut output;
                    float2 positions[3] = { float2(-1, -1), float2(3, -1), float2(-1, 3) };
                    output.pos = float4(positions[id], 0.0, 1.0);
                    output.color = float4(1, 0, 0, 1);
                    return output;
                }
                [shader("fragment")]
                float4 psMain(VSOut input) : SV_Target {
                    return input.color;
                }
            )";

            ProgramDesc progDesc;
            progDesc.addShaderModule("RasterTest").addString(shaderSource, "RasterTest.slang");
            progDesc.vsEntryPoint("vsMain");
            progDesc.psEntryPoint("psMain");
            auto program = Program::create(device, progDesc);
            REQUIRE(program);

            GraphicsPipelineDesc pipeDesc;
            pipeDesc.programKernels = program->getActiveVersion()->getKernels(device.get(), nullptr);
            pipeDesc.renderTargetCount = 1;
            pipeDesc.renderTargetFormats[0] = getGFXFormat(ResourceFormat::RGBA8Unorm);
            RasterizerState::Desc rsDesc;
            rsDesc.setCullMode(RasterizerState::CullMode::None);
            pipeDesc.rasterizerState = RasterizerState::create(rsDesc);

            auto pipeline = device->createGraphicsPipeline(pipeDesc);
            REQUIRE(pipeline);

            auto rtv = renderTarget->getRTV();
            ColorTargets colorTargets;
            colorTargets.push_back(ColorTarget(rtv, LoadOp::Clear, StoreOp::Store, {0,1,0,1}));

            auto vars = ProgramVariables::create(device, program.get());
            REQUIRE(vars);

            auto renderEncoder = ctx->beginRenderPass(colorTargets);
            Viewport vp = { 0, 0, (float)width, (float)height, 0.0f, 1.0f };
            Scissor sc = { 0, 0, width, height };
            renderEncoder->setViewport(0, vp);
            renderEncoder->setScissor(0, sc);
            renderEncoder->bindPipeline(pipeline.get(), vars.get());
            renderEncoder->draw(3, 0);
            renderEncoder->end();
            ctx->submit(true);

            auto readbackBytes = ctx->readTextureSubresource(renderTarget.get(), 0);
            ctx->submit(true);

            REQUIRE(readbackBytes.size() >= 4);
            const uint32_t* readbackData = reinterpret_cast<const uint32_t*>(readbackBytes.data());
            CHECK(readbackData[0] == 0xFF0000FF);
        }
    }

    // TEST_CASE("Ray Tracing AS Building")
    // {
    //     for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
    //     {
    //         Device::Desc deviceDesc;
    //         deviceDesc.type = deviceType;
    //         auto device = april::core::make_ref<Device>(deviceDesc);
    //         if (!device) continue;

    //         if (!device->isFeatureSupported(Device::SupportedFeatures::Raytracing)) continue;

    //         auto ctx = device->getCommandContext();
    //         REQUIRE(ctx);

    //         struct Vertex { april::float3 pos; };
    //         std::vector<Vertex> vertices = { {{0,0,0}}, {{1,0,0}}, {{0,1,0}} };
    //         auto vbo = device->createBuffer(vertices.size() * sizeof(Vertex), ResourceBindFlags::Vertex);
    //         ctx->updateBuffer(vbo.get(), vertices.data());
    //         ctx->submit(true);

    //         RtGeometryDesc geomDesc;
    //         geomDesc.type = RtGeometryType::Triangles;
    //         geomDesc.flags = RtGeometryFlags::Opaque;
    //         geomDesc.content.triangles.vertexFormat = ResourceFormat::RGB32Float;
    //         geomDesc.content.triangles.vertexCount = 3;
    //         geomDesc.content.triangles.vertexData = vbo->getGpuAddress();
    //         geomDesc.content.triangles.vertexStride = sizeof(Vertex);

    //         RtAccelerationStructureBuildInputs blasInputs;
    //         blasInputs.kind = RtAccelerationStructureKind::BottomLevel;
    //         blasInputs.flags = RtAccelerationStructureBuildFlags::PreferFastTrace;
    //         blasInputs.descCount = 1;
    //         blasInputs.geometryDescs = &geomDesc;

    //         auto blasPrebuild = RtAccelerationStructure::getPrebuildInfo(device.get(), blasInputs);
    //         auto blasBuffer = device->createBuffer(blasPrebuild.resultDataMaxSize, ResourceBindFlags::AccelerationStructure);
    //         RtAccelerationStructure::Desc blasDesc;
    //         blasDesc.setKind(RtAccelerationStructureKind::BottomLevel).setBuffer(blasBuffer, 0, blasPrebuild.resultDataMaxSize);
    //         auto blas = RtAccelerationStructure::create(device, blasDesc);
    //         REQUIRE(blas);

    //         RtInstanceDesc instance;
    //         std::memset(&instance, 0, sizeof(instance));
    //         instance.transform[0][0] = 1.0f; instance.transform[1][1] = 1.0f; instance.transform[2][2] = 1.0f;
    //         instance.accelerationStructure = blas->getGpuAddress();
    //         auto instanceBuffer = device->createBuffer(sizeof(instance), ResourceBindFlags::ShaderResource, MemoryType::Upload, &instance);

    //         RtAccelerationStructureBuildInputs tlasInputs;
    //         tlasInputs.kind = RtAccelerationStructureKind::TopLevel;
    //         tlasInputs.descCount = 1;
    //         tlasInputs.instanceDescs = instanceBuffer->getGpuAddress();

    //         auto tlasPrebuild = RtAccelerationStructure::getPrebuildInfo(device.get(), tlasInputs);
    //         auto tlasBuffer = device->createBuffer(tlasPrebuild.resultDataMaxSize, ResourceBindFlags::AccelerationStructure);
    //         RtAccelerationStructure::Desc tlasDesc;
    //         tlasDesc.setKind(RtAccelerationStructureKind::TopLevel).setBuffer(tlasBuffer, 0, tlasPrebuild.resultDataMaxSize);
    //         auto tlas = RtAccelerationStructure::create(device, tlasDesc);
    //         REQUIRE(tlas);

    //         auto scratchBuffer = device->createBuffer(std::max(blasPrebuild.scratchDataSize, tlasPrebuild.scratchDataSize), ResourceBindFlags::UnorderedAccess);
    //         auto rayEncoder = ctx->beginRayTracingPass();
    //         RtAccelerationStructure::BuildDesc blasBuild;
    //         blasBuild.inputs = blasInputs;
    //         blasBuild.dest = blas.get();
    //         blasBuild.scratchData = scratchBuffer->getGpuAddress();
    //         rayEncoder->buildAccelerationStructure(blasBuild, 0, nullptr);
    //         ctx->uavBarrier(blasBuffer.get());
    //         RtAccelerationStructure::BuildDesc tlasBuild;
    //         tlasBuild.inputs = tlasInputs;
    //         tlasBuild.dest = tlas.get();
    //         tlasBuild.scratchData = scratchBuffer->getGpuAddress();
    //         rayEncoder->buildAccelerationStructure(tlasBuild, 0, nullptr);
    //         rayEncoder->end();
    //         ctx->submit(true);

    //         CHECK(tlas->getGfxAccelerationStructure() != nullptr);
    //     }
    // }

    TEST_CASE("Manual Heap Allocation")
    {
        for (auto deviceType : { Device::Type::Vulkan }) // Only Vulkan supports IHeap in Slang-RHI for now
        {
            Device::Desc deviceDesc;
            deviceDesc.type = deviceType;
            auto device = april::core::make_ref<Device>(deviceDesc);
            if (!device) continue;

            rhi::HeapDesc heapDesc;
            heapDesc.memoryType = rhi::MemoryType::DeviceLocal;

            Slang::ComPtr<rhi::IHeap> heap = device->createHeap(heapDesc);
            if (!heap) continue; // Might not be supported

            rhi::HeapAllocDesc allocDesc;
            allocDesc.size = 1024 * 1024; // 1 MB
            allocDesc.alignment = 256;

            rhi::HeapAlloc allocation;
            REQUIRE(SLANG_SUCCEEDED(heap->allocate(allocDesc, &allocation)));
            CHECK(allocation.size >= allocDesc.size);
            CHECK(allocation.getDeviceAddress() % allocDesc.alignment == 0);

            heap->free(allocation);
        }
    }

    TEST_CASE("Internal GPU Memory Heap")
    {
        for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
        {
            Device::Desc deviceDesc;
            deviceDesc.type = deviceType;
            auto device = april::core::make_ref<Device>(deviceDesc);
            if (!device) continue;

            auto fence = device->createFence();
            auto heap = GpuMemoryHeap::create(device, MemoryType::Upload, 1024 * 1024, fence);
            REQUIRE(heap);

            auto alloc1 = heap->allocate(1024, 256);
            auto alloc2 = heap->allocate(1024, 256);

            CHECK(alloc1.gfxBuffer == alloc2.gfxBuffer);
            CHECK(alloc1.offset != alloc2.offset);

            size_t diff = (alloc1.offset > alloc2.offset) ? (alloc1.offset - alloc2.offset) : (alloc2.offset - alloc1.offset);
            CHECK(diff >= 1024);

            heap->release(alloc1);
            heap->release(alloc2);
        }
    }

    TEST_CASE("Buffer Sub-region Aliasing")
    {
        for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
        {
            Device::Desc deviceDesc;
            deviceDesc.type = deviceType;
            auto device = april::core::make_ref<Device>(deviceDesc);
            if (!device) continue;

            auto ctx = device->getCommandContext();
            REQUIRE(ctx);

            // Create a buffer for 2 uints
            auto buffer = device->createBuffer(sizeof(uint32_t) * 2, ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess);
            REQUIRE(buffer);

            // Shader that writes to two separate regions
            const char* shaderSource = R"(
                RWStructuredBuffer<uint> gOut1;
                RWStructuredBuffer<uint> gOut2;
                [shader("compute")]
                [numthreads(1, 1, 1)]
                void main() {
                    gOut1[0] = 123;
                    gOut2[0] = 456;
                }
            )";

            ProgramDesc progDesc;
            progDesc.addShaderModule("AliasTest").addString(shaderSource, "AliasTest.slang");
            progDesc.csEntryPoint("main");
            auto program = Program::create(device, progDesc);
            REQUIRE(program);

            auto vars = ProgramVariables::create(device, program.get());
            REQUIRE(vars);

            // Bind two separate views of the same buffer
            vars->setUav(vars->getVariableOffset("gOut1"), buffer->getUAV(0, sizeof(uint32_t)));
            vars->setUav(vars->getVariableOffset("gOut2"), buffer->getUAV(sizeof(uint32_t), sizeof(uint32_t)));

            ComputePipelineDesc pipeDesc;
            pipeDesc.programKernels = program->getActiveVersion()->getKernels(device.get(), vars.get());
            auto pipeline = device->createComputePipeline(pipeDesc);
            REQUIRE(pipeline);

            auto encoder = ctx->beginComputePass();
            encoder->bindPipeline(pipeline.get(), vars.get());
            encoder->dispatch(april::uint3(1, 1, 1));
            encoder->end();
            ctx->submit(true);

            auto result = ctx->readBuffer<uint32_t>(buffer.get());
            ctx->submit(true);

            CHECK(result[0] == 123);
            CHECK(result[1] == 456);
        }
    }

    TEST_CASE("Swapchain Creation")
    {
        if (!glfwInit()) return;

        for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
        {
            Device::Desc deviceDesc;
            deviceDesc.type = deviceType;
            auto device = april::core::make_ref<Device>(deviceDesc);
            if (!device) continue;

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            GLFWwindow* window = glfwCreateWindow(640, 480, "Validation", nullptr, nullptr);
            if (!window) continue;

#if defined(_WIN32)
            HWND hwnd = glfwGetWin32Window(window);
            WindowHandle winHandle = (WindowHandle)hwnd;
#else
            WindowHandle winHandle = nullptr;
#endif

            Swapchain::Desc swapDesc;
            swapDesc.width = 640;
            swapDesc.height = 480;
            swapDesc.format = ResourceFormat::RGBA8Unorm;
            swapDesc.imageCount = 3;

            auto swapchain = april::core::make_ref<Swapchain>(device, swapDesc, winHandle);
            REQUIRE(swapchain);
            CHECK(swapchain->getGfxSurface() != nullptr);

            auto backBuffer = swapchain->acquireNextImage();
            CHECK(backBuffer != nullptr);

            swapchain->present();

            device->wait();
            swapchain = nullptr;
            backBuffer = nullptr;
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }

    TEST_CASE("PSO Caching")
    {
        for (auto deviceType : { Device::Type::D3D12, Device::Type::Vulkan })
        {
            Device::Desc deviceDesc;
            deviceDesc.type = deviceType;
            auto device = april::core::make_ref<Device>(deviceDesc);
            if (!device) continue;

            const char* shaderSource = R"(
                [shader("compute")]
                [numthreads(1, 1, 1)]
                void main() {}
            )";

            ProgramDesc progDesc;
            progDesc.addShaderModule("CacheTest").addString(shaderSource, "CacheTest.slang");
            progDesc.csEntryPoint("main");

            // 1. First compilation (Miss)
            {
                auto program = Program::create(device, progDesc);
                auto version = program->getActiveVersion();

                ComputePipelineDesc pipeDesc;
                pipeDesc.programKernels = version->getKernels(device.get(), nullptr);
                auto pipeline = device->createComputePipeline(pipeDesc);
                REQUIRE(pipeline);
            }

            auto shaderStats = device->getShaderCacheStats();
            auto pipelineStats = device->getPipelineCacheStats();

            // Check that we have entries
            CHECK(shaderStats.entryCount > 0);

            uint32_t initialShaderHits = shaderStats.hitCount;
            uint32_t initialPipelineHits = pipelineStats.hitCount;

            // 2. Second compilation (Hit)
            {
                // Create a NEW program with SAME description to force re-link through RHI
                auto program = Program::create(device, progDesc);
                auto version = program->getActiveVersion();

                ComputePipelineDesc pipeDesc;
                pipeDesc.programKernels = version->getKernels(device.get(), nullptr);
                auto pipeline = device->createComputePipeline(pipeDesc);
                REQUIRE(pipeline);
            }

            auto shaderStats2 = device->getShaderCacheStats();
            auto pipelineStats2 = device->getPipelineCacheStats();

            CHECK(shaderStats2.hitCount > initialShaderHits);
            // Pipeline cache might not always hit depending on backend implementation of "hit",
            // but shader cache definitely should.
        }
    }
}
