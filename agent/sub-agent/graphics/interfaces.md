# Graphics Public Interfaces

## Purpose
Rendering and RHI abstractions plus materials and shader program support.

## Usage Patterns
- Use `Device` to create resources and pipelines; `Swapchain` handles presentation.
- Record GPU work with `CommandContext` and pass encoders, then `submit()`.
- Use `Program`/`ProgramVariables` to bind shader parameters and reflection.
- Use `MaterialSystem` + `IMaterial` for material data and bindings.

## Public Header Index
- `graphics/camera/camera.hpp` — ICamera base class with view/projection matrices and viewport management.
- `graphics/material/i-material.hpp` — Material interface for GPU data and texture bindings.
- `graphics/material/basic-material.hpp` — Falcor-style host base abstraction for shared non-layered material data/texture semantics.
- `graphics/material/material-system.hpp` — Material registry and GPU buffer manager.
- `graphics/material/standard-material.hpp` — Default PBR material implementation.
- `graphics/program/program-variables.hpp` — Program variable binding and ray tracing variable management.
- `graphics/program/program.hpp` — Shader program description, compilation, and management (Falcor-style).
- `graphics/resources/static-mesh.hpp` — GPU-side static mesh representation with submesh draw ranges.
- `graphics/rhi/command-context.hpp` — Command recording context and pass encoders for render/compute/ray tracing.
- `graphics/rhi/depth-stencil-state.hpp` — Depth/stencil state descriptor and immutable state object.
- `graphics/rhi/graphics-pipeline.hpp` — Graphics pipeline descriptor and pipeline object.
- `graphics/rhi/rasterizer-state.hpp` — Rasterizer state descriptor (culling, fill, scissor, etc.).
- `graphics/rhi/render-device.hpp` — GPU device abstraction for resource creation and capability queries.
- `graphics/rhi/render-target.hpp` — Render target attachment descriptors for render passes.
- `graphics/rhi/resource-views.hpp` — Resource view descriptors, bind flags, and base view types.
- `graphics/rhi/sampler.hpp` — Sampler state descriptor and object.
- `graphics/rhi/swapchain.hpp` — Swapchain for window presentation.
- `graphics/rhi/texture.hpp` — Texture resource abstraction and view accessors.
- `graphics/rhi/vertex-array-object.hpp` — Vertex array object binding buffers and input layout.

## Header Details

### graphics/camera/camera.hpp
Location: `engine/graphics/source/graphics/camera/camera.hpp`
Include: `#include <graphics/camera/camera.hpp>`

Purpose: ICamera base class with view/projection matrices and viewport management.

Key Types: `EProjectionType`, `ICamera`
Key APIs: `ICamera::setViewportSize()`, `ICamera::setPerspective()`, `ICamera::setPosition()`, `ICamera::getViewMatrix()`

Usage Notes:
- Derive and override `onUpdate()` for camera behavior.
- Call `setViewportSize()` when the viewport changes.

Used By: `editor`

### graphics/material/i-material.hpp
Location: `engine/graphics/source/graphics/material/i-material.hpp`
Include: `#include <graphics/material/i-material.hpp>`

Purpose: Material interface for GPU data and texture bindings.

Key Types: `ShaderVariable`, `Texture`, `IMaterial`, `MaterialUpdateFlags`
Key APIs: `IMaterial::update(...)`, `IMaterial::getDataBlob()`, `IMaterial::registerUpdateCallback(...)`, `IMaterial::setDefaultTextureSampler(...)`, `IMaterial::getDefaultTextureSampler()`, `IMaterial::setTexture(...)`, `IMaterial::getTexture(...)`, `IMaterial::getTextureSlotInfo(...)`, `IMaterial::getTypeConformances()`, `IMaterial::getDefines()`

Usage Notes:
- Implement this interface for new material types.
- Use with `MaterialSystem` to populate GPU buffers.
- Material update flow is Falcor-style: edits mark update flags, and `MaterialSystem::update()` consumes per-material updates and uploads changed data.
- `IMaterial` no longer exposes JSON serialization APIs; runtime contract stays strictly render-facing.

Used By: `scene`

### graphics/material/basic-material.hpp
Location: `engine/graphics/source/graphics/material/basic-material.hpp`
Include: `#include <graphics/material/basic-material.hpp>`

Purpose: Shared host-side base implementation for non-layered material behavior (flags, texture slots, descriptor handles, and common data wiring) aligned with Falcor BasicMaterial architecture.

Key Types: `BasicMaterial`
Key APIs: `BasicMaterial::bindTextures(...)`, `BasicMaterial::setDescriptorHandles(...)`, `BasicMaterial::writeCommonData(...)`

Usage Notes:
- Derive concrete materials (for example `StandardMaterial`) from this base instead of duplicating common payload/texture logic.
- This is a host abstraction only; it is not a standalone runtime `MaterialType` enum value.

Used By: `scene`

### graphics/material/material-system.hpp
Location: `engine/graphics/source/graphics/material/material-system.hpp`
Include: `#include <graphics/material/material-system.hpp>`

Purpose: Material registry and GPU buffer manager.

Key Types: `Device`, `ShaderVariable`, `MaterialSystem`, `MaterialStats`
Key APIs: `MaterialSystem::addMaterial()`, `MaterialSystem::update()`, `MaterialSystem::bindShaderData()`, `MaterialSystem::getTypeConformances()`, `MaterialSystem::registerTextureDescriptor()`, `MaterialSystem::registerSamplerDescriptor()`, `MaterialSystem::registerBufferDescriptor()`, `MaterialSystem::registerTexture3DDescriptor()`, `MaterialSystem::addTexture()/replaceTexture()`, `MaterialSystem::enqueueDeferredTextureLoad()`, `MaterialSystem::setUdimIndirectionBuffer()/setUdimIndirectionEnabled()/setLightProfileEnabled()`, `MaterialSystem::addSampler()/replaceSampler()`, `MaterialSystem::addBuffer()/replaceBuffer()`, `MaterialSystem::addTexture3D()/replaceTexture3D()`, `MaterialSystem::setMaterialParamLayout()`, `MaterialSystem::setSerializedMaterialParams()`, `MaterialSystem::removeDuplicateMaterials()`, `MaterialSystem::optimizeMaterials()`, `MaterialSystem::getShaderDefines()`, `MaterialSystem::getStats()`, `MaterialTextureManager::registerTexture()/replaceTexture()/resolveDeferred()`, `MaterialTextureAnalyzer::analyze()/canPrune()`

Usage Notes:
- Call `update()` after modifying material data.
- Material GPU ABI uses Falcor-style `generated::MaterialDataBlob` (`MaterialHeader` 16B bit-packed + `MaterialPayload` 112B).
- Descriptor handles use `MaterialSystem::DescriptorHandle`, with `kInvalidDescriptorHandle` as the fallback for missing resources.
- Material type metadata is exposed through `MaterialTypeRegistry` (`resolveTypeId()`, `resolveTypeName()`) and per-material lookup via `MaterialSystem::getMaterialTypeId()`.
- Descriptor capacities are computed from material metadata and fixed Falcor-style limits; call `getShaderDefines()` to sync shader compilation contract.
- `bindShaderData()` is the single host authority for binding material-system resources (`materialCount`, `materialData`, descriptor arrays) to shader variables.
- Shader-side material contracts now require host-provided `FALCOR_MATERIAL_INSTANCE_SIZE` and `MATERIAL_SYSTEM_*` define set (including optional 3D/UDIM/light-profile toggles).
- Shader-side `IMaterialInstance` now follows Falcor-style template signatures for eval/sample paths (`ISampleGenerator`-based), and `BSDFProperties` carries split albedo channels plus guide-normal/specular reflectance fields.
- Shader module set now includes Falcor-style parameter serialization/layout scaffolding (`material-param-layout.slang`, `serialized-material-params.slang`) for future host parity wiring.
- Phase-function conformance files live at `engine/graphics/shader/material/phase/*.slang`; host injects `IPhaseFunction` conformances via `MaterialSystem::getTypeConformances()`.
- Material system now exposes host-side parameter-layout payload wiring for shader contracts (`materialParamLayoutEntries`, serialized param entries/data) and consumes deferred texture loaders during update lifecycle.
- Material define conflicts are treated as fatal at runtime, matching strict Falcor-style contract enforcement expectations.
- Scene shading path (`scene-mesh.slang`) now consumes `MaterialSystem::evalPhaseFunction()` for transmissive response contribution.
- Use `getStats()` to retrieve Falcor-style material/texture statistics.

Used By: `scene`

### Material Extension Workflow
Location: `engine/graphics/source/graphics/material/*`, `engine/graphics/shader/material/*`

Steps:
- Add host material class implementing `IMaterial` only when it maps to Falcor material architecture contracts.
- Register/resolve stable type id through `MaterialTypeRegistry` in `MaterialSystem`.
- Update shared Slang schema in `engine/graphics/shader/shared/material/*.shared.slang` when changing exported ABI contracts, then regenerate headers via module codegen.
- Keep file layout Falcor-aligned: split `i-material.slang` / `i-material-instance.slang`, and keep material implementation files aligned to active Falcor material families.
- Add Slang material object implementing `IMaterial` plus a material instance implementing `IMaterialInstance`.
- Route instance creation through `MaterialSystem` dynamic dispatch (`createDynamicObject<IMaterial, MaterialDataBlob>`) and extension-style factory helpers in `material-factory.slang`.
- Use `texture-sampler.slang` + `material-instance-hints.slang` contracts for `(sd, lod, hints)` instance construction signatures.
- Ensure host `getTypeConformances()` contributes `<Type, IMaterial>` and `<TypeInstance, IMaterialInstance>` mappings and `getShaderModules()` supplies type-deduplicated module list.

### graphics/material/standard-material.hpp
Location: `engine/graphics/source/graphics/material/standard-material.hpp`
Include: `#include <graphics/material/standard-material.hpp>`

Purpose: Default PBR material implementation.

Key Types: `Device`, `StandardMaterial`, `DiffuseBRDFModel`
Key APIs: `StandardMaterial::createFromAsset(...)`, `StandardMaterial::DiffuseBRDFModel`

Usage Notes:
- Use `createFromAsset()` to build from `asset::MaterialAsset`.

Used By: `scene`

### graphics/program/program-variables.hpp
Location: `engine/graphics/source/graphics/program/program-variables.hpp`
Include: `#include <graphics/program/program-variables.hpp>`

Purpose: Program variable binding and ray tracing variable management.

Key Types: `Device`, `ProgramVariables`, `RayTracingPipeline`, `RtProgramVariables`, `EntryPointGroupInfo`
Key APIs: `ProgramVariables::create(...)`, `RtProgramVariables::create(...)`, `RtProgramVariables::prepareShaderTable(...)`

Usage Notes:
- Create variables from a `Program` or `ProgramReflection` before binding.

Used By: `editor`, `scene`

### graphics/program/program.hpp
Location: `engine/graphics/source/graphics/program/program.hpp`
Include: `#include <graphics/program/program.hpp>`

Purpose: Shader program description, compilation, and management (Falcor-style).

Key Types: `RayTracingPipeline`, `RtProgramVariables`, `TypeConformance`, `HashFunction`, `TypeConformanceList`, `SlangCompilerFlags`, `ProgramDesc`, `ShaderID`, `ShaderSource`, `Type`, `ShaderModule`, `EntryPoint`, `EntryPointGroup`, `Program`, `ProgramManager`, `ProgramVersion`, `ParameterBlockReflection`, `ProgramVersionKey`
Key APIs: `ProgramDesc`, `Program`, `ProgramManager`, `TypeConformanceList`

Usage Notes:
- Build `ProgramDesc` with modules/entry points; compile via `ProgramManager`.
- Use `TypeConformanceList` to select interface implementations.
- Material conformance failures are surfaced through semantic Slang compile/link diagnostics, without path-name heuristic link gating.

Used By: `scene`

### graphics/resources/static-mesh.hpp
Location: `engine/graphics/source/graphics/resources/static-mesh.hpp`
Include: `#include <graphics/resources/static-mesh.hpp>`

Purpose: GPU-side static mesh representation with submesh draw ranges.

Key Types: `StaticMesh`, `DrawRange`
Key APIs: `StaticMesh`, `StaticMesh::DrawRange`

Usage Notes:
- Pair with `VertexArrayObject` and draw per submesh range.

Used By: `scene`

### graphics/rhi/command-context.hpp
Location: `engine/graphics/source/graphics/rhi/command-context.hpp`
Include: `#include <graphics/rhi/command-context.hpp>`

Purpose: Command recording context and pass encoders for render/compute/ray tracing.

Key Types: `Profiler`, `CommandContext`, `TEncoder`, `PassEncoderBase`, `RtAccelerationStructureCopyMode`, `Viewport`, `Scissor`, `RenderPassEncoder`, `ComputePassEncoder`, `RayTracingPassEncoder`, `SubmissionPayload`, `ReadTextureTask`
Key APIs: `CommandContext::beginRenderPass()`, `CommandContext::beginComputePass()`, `CommandContext::beginRayTracingPass()`, `CommandContext::finish()/submit()`, `RenderPassEncoder::draw()/drawIndexed()`

Usage Notes:
- Begin a pass, record commands, then call `end()` on the encoder.
- Use `submit()` to execute immediately or `finish()` for deferred submission.

Used By: `editor`, `runtime`, `scene`

### graphics/rhi/depth-stencil-state.hpp
Location: `engine/graphics/source/graphics/rhi/depth-stencil-state.hpp`
Include: `#include <graphics/rhi/depth-stencil-state.hpp>`

Purpose: Depth/stencil state descriptor and immutable state object.

Key Types: `DepthStencilState`, `Face`, `StencilOp`, `StencilDesc`, `Desc`
Key APIs: `DepthStencilState::Desc`, `DepthStencilState::create(...)`

Usage Notes:
- Configure `Desc` then attach to `GraphicsPipelineDesc`.

Used By: `scene`

### graphics/rhi/graphics-pipeline.hpp
Location: `engine/graphics/source/graphics/rhi/graphics-pipeline.hpp`
Include: `#include <graphics/rhi/graphics-pipeline.hpp>`

Purpose: Graphics pipeline descriptor and pipeline object.

Key Types: `ProgramKernels`, `GraphicsPipelineDesc`, `PrimitiveType`, `GraphicsPipeline`
Key APIs: `GraphicsPipelineDesc`, `GraphicsPipeline`

Usage Notes:
- Fill `GraphicsPipelineDesc` with layouts, program kernels, and state objects.
- Bind via `RenderPassEncoder::bindPipeline()`.

Used By: `scene`

### graphics/rhi/rasterizer-state.hpp
Location: `engine/graphics/source/graphics/rhi/rasterizer-state.hpp`
Include: `#include <graphics/rhi/rasterizer-state.hpp>`

Purpose: Rasterizer state descriptor (culling, fill, scissor, etc.).

Key Types: `RasterizerState`, `CullMode`, `FillMode`, `Desc`
Key APIs: `RasterizerState::Desc`, `RasterizerState::create(...)`, `RasterizerState::CullMode`

Usage Notes:
- Configure culling/fill mode and attach to `GraphicsPipelineDesc`.

Used By: `scene`

### graphics/rhi/render-device.hpp
Location: `engine/graphics/source/graphics/rhi/render-device.hpp`
Include: `#include <graphics/rhi/render-device.hpp>`

Purpose: GPU device abstraction for resource creation and capability queries.

Key Types: `Profiler`, `ProgramManager`, `Program`, `GpuProfiler`, `AftermathContext`, `ShaderVariable`, `CudaDevice`, `AdapterLUID`, `AdapterInfo`, `Device`, `Type`, `Desc`, `Info`, `Limits`, `CacheStats`, `SupportedFeatures`, `ResourceRelease`
Key APIs: `Device::Desc`, `Device::getGPUs(...)`, `Device::createBuffer()/createTexture*()`, `Device::createGraphicsPipeline(...)`, `Device::createSampler()/createFence()`

Usage Notes:
- Use `Device` as the root owner for GPU resources and pipelines.
- Select backend and debug options via `Device::Desc`.

Used By: `editor`, `runtime`, `scene`

### graphics/rhi/render-target.hpp
Location: `engine/graphics/source/graphics/rhi/render-target.hpp`
Include: `#include <graphics/rhi/render-target.hpp>`

Purpose: Render target attachment descriptors for render passes.

Key Types: `LoadOp`, `StoreOp`, `Attachment`, `ColorTarget`, `DepthStencilTarget`
Key APIs: `ColorTarget`, `DepthStencilTarget`, `LoadOp`, `StoreOp`

Usage Notes:
- Pass color/depth targets into `CommandContext::beginRenderPass()`.

Used By: `runtime`

### graphics/rhi/resource-views.hpp
Location: `engine/graphics/source/graphics/rhi/resource-views.hpp`
Include: `#include <graphics/rhi/resource-views.hpp>`

Purpose: Resource view descriptors, bind flags, and base view types.

Key Types: `Device`, `Resource`, `Texture`, `Buffer`, `ResourceBindFlags`, `ResourceViewDimension`, `BufferCreationMode`, `ResourceViewInfo`, `ResourceViewDesc`, `TextureInfo`, `BufferInfo`, `ResourceView`, `TextureView`, `BufferView`
Key APIs: `ResourceViewDesc`, `ResourceBindFlags`, `ResourceView`, `ResourceViewDesc::AsStructuredBuffer()`

Usage Notes:
- Use helper constructors on `ResourceViewDesc` to build SRV/UAV/RTV/DSV views.

Used By: `editor`, `runtime`, `scene`

### graphics/rhi/sampler.hpp
Location: `engine/graphics/source/graphics/rhi/sampler.hpp`
Include: `#include <graphics/rhi/sampler.hpp>`

Purpose: Sampler state descriptor and object.

Key Types: `Device`, `TextureFilteringMode`, `TextureAddressingMode`, `TextureReductionMode`, `Sampler`, `Desc`
Key APIs: `Sampler::Desc`, `TextureFilteringMode`, `TextureAddressingMode`, `TextureReductionMode`

Usage Notes:
- Configure filtering/addressing in `Sampler::Desc` then create a Sampler.

Used By: `scene`

### graphics/rhi/swapchain.hpp
Location: `engine/graphics/source/graphics/rhi/swapchain.hpp`
Include: `#include <graphics/rhi/swapchain.hpp>`

Purpose: Swapchain for window presentation.

Key Types: `Device`, `Swapchain`, `Desc`
Key APIs: `Swapchain::present()`, `Swapchain::acquireNextImage()`, `Swapchain::resize()`

Usage Notes:
- Call `acquireNextImage()` each frame and `present()` after rendering.
- Use `resize()` when the window size changes.

Used By: `runtime`

### graphics/rhi/texture.hpp
Location: `engine/graphics/source/graphics/rhi/texture.hpp`
Include: `#include <graphics/rhi/texture.hpp>`

Purpose: Texture resource abstraction and view accessors.

Key Types: `Sampler`, `CommandContext`, `TextureUsage`, `Texture`, `SubresourceLayout`, `Device`
Key APIs: `TextureUsage`, `Texture::getSRV()/getUAV()`, `Texture::createFromFile()`

Usage Notes:
- Use `TextureUsage` flags to describe intended usage (SRV/UAV/RTV/etc).
- Use `getSRV()`/`getUAV()` to obtain default views.

Used By: `runtime`, `scene`

### graphics/rhi/vertex-array-object.hpp
Location: `engine/graphics/source/graphics/rhi/vertex-array-object.hpp`
Include: `#include <graphics/rhi/vertex-array-object.hpp>`

Purpose: Vertex array object binding buffers and input layout.

Key Types: `VertexArrayObject`, `Topology`, `ElementDesc`, `CommandContext`
Key APIs: `VertexArrayObject::create(...)`, `VertexArrayObject::Topology`

Usage Notes:
- Create with a `VertexLayout` and vertex/index buffers, then bind in render pass.

Used By: `editor`

