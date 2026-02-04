# April2 Engine - Rendering Architecture Understanding

*Last Updated: 2026-02-04*

This document provides a comprehensive understanding of the April2 Engine's rendering architecture, generated from codebase exploration. Use this as reference for future rendering work.

---

## Module Structure

```
engine/
├── core/          - Foundation (logging, math, file I/O, window)
├── asset/         - Asset management
├── graphics/      - RHI, shaders, rendering
├── scene/         - ECS (Entity-Component-System)
├── runtime/       - Engine loop, main application
├── ui/            - ImGui integration
└── editor/        - Editor application
```

**Dependency Chain**: `core → graphics → runtime → ui → editor`
**Scene**: Independent, depends only on `core`

---

## 1. Runtime Module - Main Loop

### Entry Point: Engine Class

**Location**: `engine/runtime/source/runtime/engine.hpp/cpp`

**Key Methods**:
- `init(EngineConfig const&)` - Initialize all systems
- `run()` - Main loop (while m_running)
- `renderFrame(float deltaTime)` - Per-frame rendering
- `shutdown()` - Cleanup

### Main Loop Flow

```cpp
auto Engine::run() -> void
{
    while (m_running)
    {
        auto now = Clock::now();
        float deltaTime = (now - lastTime).count();
        lastTime = now;

        Input::beginFrame();
        m_window->onEvent();
        renderFrame(deltaTime);
    }
}
```

### Render Frame Flow

**Location**: `engine/runtime/source/runtime/engine.cpp:75-177`

```
1. Check swapchain dirty (resize handling)
2. Acquire backbuffer from swapchain
3. Call user onUpdate hook (game logic)
4. Setup render target (offscreen or backbuffer)
5. Call user onRender hook (or clear target if absent)
6. ImGuiLayer begins frame (if enabled)
7. Call user onUI hook
8. SceneRenderer::render (if scene graph exists)
9. SceneRenderer composite to output
10. ImGuiLayer renders UI
11. Transition backbuffer to Present
12. Submit commands to GPU
13. Swapchain present
14. Device endFrame (resource cleanup)
```

**Key State**:
- `m_window` - Window handle (GLFW wrapper)
- `m_device` - Graphics device (D3D12/Vulkan)
- `m_swapchain` - Presentation surface
- `m_context` - Command context for recording
- `m_renderer` - SceneRenderer instance
- `m_assetManager` - Asset loading
- `m_imguiLayer` - UI system

### Hooks for User Code

```cpp
struct EngineHooks
{
    std::function<void()> onInit;
    std::function<void()> onShutdown;
    std::function<void(float)> onUpdate;           // Game logic
    std::function<void(CommandContext*, TextureView*)> onRender;  // Custom rendering
    std::function<void()> onUI;                    // ImGui UI
};
```

---

## 2. Graphics Module - Rendering System

### Device: RHI Abstraction

**Location**: `engine/graphics/source/graphics/rhi/render-device.hpp`

**Purpose**: Abstracts D3D12/Vulkan via slang-rhi

**Key Responsibilities**:
- Resource creation (textures, buffers, samplers)
- Pipeline creation (graphics, compute, ray tracing)
- Command context management
- Shader compilation (Slang)
- Frame synchronization

**Frame Management**:
```cpp
static constexpr uint32_t kInFlightFrameCount = 3;

auto beginFrame() -> void;      // Start new frame
auto endFrame() -> void;        // Submit work, cleanup resources
```

### CommandContext: GPU Command Recording

**Location**: `engine/graphics/source/graphics/rhi/command-context.hpp`

**Encoders**:
1. **RenderPassEncoder** - Graphics commands
   - `bindPipeline()`, `draw()`, `drawIndexed()`, `drawInstanced()`
   - `setViewport()`, `setScissor()`, `setVao()`
   - `blit()`, `resolveResource()`

2. **ComputePassEncoder** - Compute commands
   - `bindPipeline()`, `dispatch()`, `dispatchIndirect()`

3. **RayTracingPassEncoder** - Ray tracing commands
   - `bindPipeline()`, `raytrace()`
   - `buildAccelerationStructure()`

**Command Flow**:
```cpp
// Begin render pass
auto encoder = ctx->beginRenderPass(colorTargets, depthTarget);

// Bind pipeline and resources
encoder->setViewport(0, viewport);
encoder->bindPipeline(pipeline, programVars);
encoder->setVao(vao);

// Draw
encoder->drawIndexed(indexCount, indexOffset, baseVertex);

// End pass
encoder->end();

// Submit to GPU
ctx->submit();
```

### Texture: GPU Texture Resources

**Location**: `engine/graphics/source/graphics/rhi/texture.hpp`

**Types**: 1D, 2D, 3D, Cube, Multisampled

**Usage Flags**:
- `ShaderResource` - Sample in shaders (SRV)
- `UnorderedAccess` - Compute writes (UAV)
- `RenderTarget` - Render pass output (RTV)
- `DepthStencil` - Depth testing (DSV)
- `Present` - Display to screen
- `CopySource/Destination`, `ResolveSource/Destination`

**View Types**:
- RTV: Render target view (`getRTV()`)
- DSV: Depth-stencil view (`getDSV()`)
- SRV: Shader resource view (`getSRV()`)

### Buffer: GPU Buffer Resources

**Location**: `engine/graphics/source/graphics/rhi/buffer.hpp`

**Memory Types**:
- `DeviceLocal` - GPU-only memory (fast)
- `Upload` - CPU write, GPU read
- `ReadBack` - GPU write, CPU read

**Usage Flags**:
- `VertexBuffer`, `IndexBuffer`, `ConstantBuffer`
- `ShaderResource` (SRV), `UnorderedAccess` (UAV)
- `IndirectArgument`, `AccelerationStructure`, `ShaderTable`

### StaticMesh: Geometry Data

**Location**: `engine/graphics/source/graphics/resources/static-mesh.hpp`

**Structure**:
```cpp
class StaticMesh
{
    core::ref<VertexArrayObject> m_vao;  // Vertex/index buffers
    std::vector<DrawRange> m_submeshes;  // Submesh info
    float3 m_boundsMin, m_boundsMax;     // AABB
};

struct DrawRange
{
    uint32_t indexOffset;
    uint32_t indexCount;
    uint32_t materialIndex;
};
```

**Loading**: `Device::createMeshFromAsset(*assetManager, *meshAsset)` (after `AssetManager::loadAsset`)

### GraphicsPipeline: Rendering State

**Location**: `engine/graphics/source/graphics/rhi/graphics-pipeline.hpp`

**Descriptor**:
- Vertex layout (input assembler)
- Shader kernels (vertex, pixel, etc.)
- Rasterizer state (cull mode, fill mode, depth bias)
- Depth-stencil state (depth test, stencil ops)
- Blend state (color blending, alpha blending)
- Primitive topology (triangles, lines, points)
- Render target formats

### Camera System

**Location**: `engine/graphics/source/graphics/camera/`

**ICamera Interface**:
- Projection types: Perspective, Orthographic
- Matrices: View, Projection, ViewProjection
- Vectors: Position, Direction, Right, Up
- Parameters: FOV, near/far planes, viewport size

**Implementations**:
- `FixedCamera` - Static camera (no movement)
- `SimpleCamera` - Basic FPS-style camera with mouse/keyboard input

---

## 3. Scene Module - ECS System

### Entity-Component-System

**Location**: `engine/scene/source/scene/ecs-core.hpp`

**Entity Type**:
```cpp
using Entity = uint32_t;
inline constexpr Entity NullEntity = 0xFFFFFFFF;
```

### SparseSet: Component Storage

**Data Structure**:
```
m_data:            [comp5, comp2, comp17, comp9]  (Dense, cache-friendly)
m_denseToEntity:   [  5,    2,    17,     9  ]    (Dense index → Entity)
m_entityToDense:   [-, -, 1, -, -, 0, -, -, -, 3, -, -, -, -, -, -, -, 2]  (Entity → Dense index)
```

**Operations**:
- `emplace(entity, args...)` - O(1) add with perfect forwarding
- `remove(entity)` - O(1) swap-and-pop removal
- `get(entity)` - O(1) component access
- `contains(entity)` - O(1) existence check

**Key Property**: Dense array enables cache-friendly iteration over all components of type T.

### Registry: ECS Manager

**EnTT-Compatible API**:
```cpp
auto create() -> Entity;                               // Create entity
auto destroy(Entity) -> void;                          // Destroy entity
auto emplace<T>(Entity, Args...) -> T&;               // Add component
auto get<T>(Entity) -> T&;                            // Get component
auto allOf<T>(Entity) const -> bool;                  // Check component
auto view<T>() -> View<T>;                            // Iterate components
```

**Storage**: `std::unordered_map<std::type_index, std::unique_ptr<ISparseSet>> m_pools`

### Standard Components

**Location**: `engine/scene/source/scene/components.hpp`

#### IDComponent
```cpp
struct IDComponent
{
    core::UUID id{};  // Unique identifier
};
```

#### TagComponent
```cpp
struct TagComponent
{
    std::string tag{};  // Human-readable name
};
```

#### TransformComponent
```cpp
struct TransformComponent
{
    // Local space
    float3 localPosition{0.f};
    float3 localRotation{0.f};  // Euler angles (radians)
    float3 localScale{1.f};

    // World space (computed)
    float4x4 worldMatrix{1.f};
    bool isDirty = true;

    auto getLocalMatrix() const -> float4x4;  // Compute local TRS matrix
};
```

#### RelationshipComponent (Hierarchy)
```cpp
struct RelationshipComponent
{
    size_t childrenCount = 0;

    Entity parent = NullEntity;
    Entity firstChild = NullEntity;
    Entity prevSibling = NullEntity;
    Entity nextSibling = NullEntity;
};
```

#### MeshRendererComponent
```cpp
struct MeshRendererComponent
{
    std::string meshAssetPath{};  // Path to .asset file
    uint32_t materialId{0};
    bool castShadows{true};
    bool receiveShadows{true};
    bool enabled{true};
};
```

#### CameraComponent
```cpp
struct CameraComponent
{
    bool isPerspective{true};
    float fov{glm::radians(45.0f)};
    float orthoSize{10.0f};
    float nearClip{0.1f};
    float farClip{1000.0f};

    uint32_t viewportWidth{1920};
    uint32_t viewportHeight{1080};

    float4x4 viewMatrix{1.0f};
    float4x4 projectionMatrix{1.0f};
    float4x4 viewProjectionMatrix{1.0f};

    bool isDirty{true};
};
```

**Hierarchy Structure**: Intrusive linked list

```
Parent
├─ firstChild → Child1 (prevSibling: null, nextSibling: Child2)
│               └─ firstChild → ...
├─ Child2 (prevSibling: Child1, nextSibling: Child3)
└─ Child3 (prevSibling: Child2, nextSibling: null)
```

### SceneGraph: High-Level Scene Manager

**Location**: `engine/scene/source/scene/scene-graph.hpp/cpp`

**Entity Lifecycle**:
```cpp
auto createEntity(std::string const& name) -> Entity;
auto destroyEntity(Entity) -> void;  // Recursive (destroys children)
```

**Hierarchy Management**:
```cpp
auto setParent(Entity child, Entity newParent) -> void;
// Unlinks from old parent, links to new parent, marks transforms dirty
```

**Transform System**:
```cpp
auto updateTransform(Entity, float4x4 const& parentMatrix, bool parentDirty) -> void;
// Recursively propagates world transforms through hierarchy
```

**Implementation Details**:
- `unlinkFromParent()` - Removes entity from parent's child list (fixes sibling links)
- `linkToParent()` - Adds entity to parent's child list (at beginning)
- `destroyEntityRecursive()` - Destroys all children first, then entity

---

## 4. SceneRenderer: ECS Renderer

**Location**: `engine/graphics/source/graphics/renderer/scene-renderer.hpp/cpp`

**Current State**:
- ECS-driven rendering via `MeshRendererComponent` + `TransformComponent`
- Active camera pulled from `SceneGraph::getActiveCamera()` (tag `MainCamera` or first camera)
- Mesh cache keyed by asset path, loaded through `AssetManager`
- Offscreen rendering to RGBA16Float + D32Float
- Simple diffuse shading (embedded Slang shaders)

**Constructor**:
1. Create graphics pipeline with vertex/pixel shaders
2. Initialize program variables and state (no hardcoded mesh or camera)

**Render Method**:
```cpp
auto render(CommandContext* ctx, scene::SceneGraph const& scene, float4 const& clearColor) -> void
{
    ensureTarget(m_width, m_height);
    ctx->resourceBarrier(m_sceneColor, RenderTarget);
    ctx->resourceBarrier(m_sceneDepth, DepthStencil);

    updateActiveCamera(scene);
    if (!m_hasActiveCamera)
    {
        return;
    }

    auto const& registry = scene.getRegistry();

    // Preload meshes into GPU cache
    for (auto const& meshComp : registry.getPool<MeshRendererComponent>()->data())
    {
        if (meshComp.enabled && !meshComp.meshAssetPath.empty())
        {
            getMeshForPath(meshComp.meshAssetPath);
        }
    }

    auto encoder = ctx->beginRenderPass(colorTarget, depthTarget);
    encoder->setViewport(0, viewport);
    encoder->setScissor(0, scissor);

    renderMeshEntities(encoder, registry);

    encoder->end();
    ctx->resourceBarrier(m_sceneColor, ShaderResource);
}
```

**Shaders** (embedded in scene-renderer.cpp):
```glsl
// Vertex Shader
struct VertexIn {
    float3 position;
    float3 normal;
};

struct VertexOut {
    float4 position : SV_Position;
    float3 normal : NORMAL;
};

cbuffer PerFrame {
    float4x4 viewProj;
    float4x4 model;
};

[shader("vertex")]
VertexOut vertexMain(VertexIn vin) {
    VertexOut vout;
    vout.position = mul(viewProj, mul(model, float4(vin.position, 1.0)));
    vout.normal = mul((float3x3)model, vin.normal);
    return vout;
}

// Pixel Shader
[shader("fragment")]
float4 fragmentMain(VertexOut pin) : SV_Target {
    float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
    float3 normal = normalize(pin.normal);
    float ndotl = max(dot(normal, lightDir), 0.0);
    float3 color = float3(0.8, 0.2, 0.2) * ndotl;
    return float4(color, 1.0);
}
```

**External Camera Support**:
- Editor viewport `SimpleCamera` is synchronized into a scene `CameraComponent` entity (`MainCamera`).
- `SceneRenderer` consumes camera matrices from `SceneGraph::getActiveCamera()`.

---

## 5. Editor Integration

### EditorApp

**Location**: `engine/editor/source/editor/editor-app.hpp/cpp`

**UI Elements**:
- Menu bar
- Hierarchy panel
- Inspector panel
- Viewport (3D scene view)

### EditorContext

```cpp
struct EditorContext
{
    float2 viewportSize;
    std::string projectName{"April"};
    EditorSceneRef scene;
    EditorSelection selection;      // Selected entity
    EditorToolState tools;          // Select/Move/Rotate/Scale
};
```

### EditorViewportElement

**Location**: `engine/editor/source/editor/element/editor-viewport.hpp/cpp`

**Functionality**:
- Contains `SimpleCamera` for navigation
- Creates a `MainCamera` entity with `CameraComponent` on attach
- Syncs `SimpleCamera` transform to `TransformComponent` each frame
- Captures mouse/keyboard input when hovered
- Sets viewport size for scene rendering
- Displays scene color texture as ImGui image
- Seeds example mesh entities if scene has no `MeshRendererComponent`

**Render Flow**:
```cpp
auto EditorViewportElement::onUIRender() -> void
{
    auto hovered = ImGui::IsWindowHovered(...);
    auto focused = ImGui::IsWindowFocused(...);
    m_camera->setInputEnabled(hovered || focused);
    m_camera->onUpdate(ImGui::GetIO().DeltaTime);

    // Sync editor camera to scene camera entity
    if (m_cameraEntity != scene::NullEntity)
    {
        auto* scene = Engine::get().getSceneGraph();
        auto& registry = scene->getRegistry();
        auto& transform = registry.get<scene::TransformComponent>(m_cameraEntity);
        transform.localPosition = m_camera->getPosition();
        transform.localRotation = {pitch, yaw, 0.0f};
        transform.isDirty = true;
    }

    ImGui::Image(Engine::get().getSceneColorSrv(), size);
}
```

### EditorHierarchyElement

**Location**: `engine/editor/source/editor/element/editor-hierarchy.hpp/cpp`

**Current State**:
- Draws scene tree from `RelationshipComponent` roots
- Recursively renders child entities via sibling links
- Uses `TagComponent` as label (fallback: `Entity <id>`)
- Updates `EditorContext::selection.entity` on click

### EditorInspectorElement

**Location**: `engine/editor/source/editor/element/editor-inspector.hpp/cpp`

**Current State**:
- Edits `TagComponent` name
- Shows `IDComponent` UUID
- Edits `TransformComponent` position/rotation/scale (marks dirty)
- Edits `MeshRendererComponent` mesh path and toggles
- Edits `CameraComponent` projection parameters (marks dirty)
- Shows `RelationshipComponent` parent/child info

---

## 6. Asset System

### AssetManager

**Location**: `engine/asset/source/asset/asset-manager.hpp`

**Initialization**:
```cpp
m_assetManager = std::make_unique<AssetManager>(
    "E:/github/April2/content",     // Content directory
    "E:/github/April2/Cache/DDC"    // Derived Data Cache
);
```

**Asset Types**:
- StaticMeshAsset (.asset files)
- TextureAsset
- ShaderAsset
- (Extensible for more types)

**Usage**:
```cpp
auto meshAsset = m_assetManager->loadAsset<asset::StaticMeshAsset>(assetPath);
auto mesh = m_device->createMeshFromAsset(*m_assetManager, *meshAsset);
auto texture = m_device->createTextureFromAsset(texPath);
```

---

## 7. Key Patterns & Conventions

### Smart Pointers
```cpp
core::ref<T>  // Shared ownership (similar to std::shared_ptr)
```

### Object System
```cpp
APRIL_OBJECT(ClassName)  // Macro for reflection support
```

### Logging
```cpp
AP_INFO("Message: {}", value);
AP_WARN("Warning: {}", value);
AP_ERROR("Error: {}", value);
```

### Math Types
```cpp
// From core/math/type.hpp
float2, float3, float4       // Vectors
float2x2, float3x3, float4x4 // Matrices
quaternion                   // Quaternions

// Inline namespace for GLM functions
using namespace april::math;
auto mat = glm::translate(identity, position);
```

### Code Style
- **East const**: `std::string const&` (not `const std::string&`)
- **Trailing return**: `auto func() -> void` (not `void func()`)
- **Almost always auto**: `auto value = func();`
- **Brace init**: `auto obj = MyClass{args};`
- **Naming**:
  - Variables: `camelCase`
  - Members: `m_camelCase`
  - Types: `PascalCase`
  - Files: `kebab-case.hpp`

---

## 8. Integration Points for Rendering

### Current State

1. **Scene ECS is rendered**: Entities with `MeshRendererComponent` are drawn by SceneRenderer
2. **Camera is component-based**: Active camera comes from `SceneGraph::getActiveCamera()`
3. **Transforms updated per frame**: Engine calls `updateTransforms()` and `updateCameras()`
4. **Editor camera is wired**: Viewport `SimpleCamera` drives `MainCamera` entity transform
5. **Editor scene tools are active**: Hierarchy/Inspector operate on live SceneGraph data

### Remaining Connections

1. **Asset reference model** → Move `MeshRendererComponent` from absolute asset paths to stable handles/UUIDs
2. **Sample scene bootstrap** → Move cube seeding out of `EditorViewportElement::onAttach()` into explicit scene setup

---

## 9. Performance Considerations

### Triple Buffering
- Device uses 3 in-flight frames (`kInFlightFrameCount = 3`)
- CPU can work ahead while GPU processes previous frames
- Synchronization via fences

### Resource State Tracking
- Manual resource barriers (not automatic)
- Transitions: `ShaderResource → RenderTarget → ShaderResource`
- Must transition before use in each context

### Cache-Friendly ECS
- Dense component arrays for iteration
- Sparse arrays for O(1) lookup
- Swap-and-pop keeps arrays packed

### Mesh Caching
- Avoid reloading same mesh multiple times
- Cache by asset path string key
- Shared ownership via `core::ref<StaticMesh>`

---

## 10. Future Extensions

### Phase 1: Basic Entity Rendering (Implemented)
- MeshRendererComponent, CameraComponent
- Entity iteration and rendering
- Transform propagation integration

### Phase 2: Material System
- MaterialComponent or MaterialAsset
- Multiple shaders/textures per entity
- PBR (Physically Based Rendering)

### Phase 3: Advanced Rendering
- Sorting (opaque front-to-back, transparent back-to-front)
- Frustum culling
- Occlusion culling
- Level-of-detail (LOD)

### Phase 4: Effects
- Shadow mapping
- Post-processing
- Particle systems
- Decals

### Phase 5: Editor Features
- Gizmos (move/rotate/scale)
- Grid rendering
- Selection outlines
- Asset browser

---

## File Reference

### Critical Paths

**Runtime Loop**:
- `engine/runtime/source/runtime/engine.hpp`
- `engine/runtime/source/runtime/engine.cpp`

**Scene ECS**:
- `engine/scene/source/scene/ecs-core.hpp`
- `engine/scene/source/scene/components.hpp`
- `engine/scene/source/scene/scene-graph.hpp`
- `engine/scene/source/scene/scene-graph.cpp`

**Graphics**:
- `engine/graphics/source/graphics/rhi/render-device.hpp`
- `engine/graphics/source/graphics/rhi/command-context.hpp`
- `engine/graphics/source/graphics/rhi/texture.hpp`
- `engine/graphics/source/graphics/rhi/buffer.hpp`
- `engine/graphics/source/graphics/resources/static-mesh.hpp`
- `engine/graphics/source/graphics/renderer/scene-renderer.hpp`
- `engine/graphics/source/graphics/renderer/scene-renderer.cpp`

**Editor**:
- `engine/editor/source/editor/editor-app.hpp`
- `engine/editor/source/editor/editor-context.hpp`
- `engine/editor/source/editor/element/editor-viewport.hpp`
- `engine/editor/source/editor/element/editor-hierarchy.hpp`
- `engine/editor/source/editor/element/editor-inspector.hpp`

---

*End of Rendering Architecture Understanding*
