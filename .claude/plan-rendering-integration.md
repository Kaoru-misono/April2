# Scene ECS + Rendering Integration Plan

## Overview

Integrate the existing Scene ECS with the rendering pipeline to enable entity-based rendering. This refactors SceneRenderer from hardcoded cube rendering to iterating entities with MeshRendererComponent and rendering them using their TransformComponent world matrices.

## Architecture Decisions

### Component Location: Scene Module
- Place `MeshRendererComponent` and `CameraComponent` in `engine/scene/source/scene/components.hpp`
- Maintains clean dependency: Scene (data) → Graphics (rendering)
- Scene module remains independent (no graphics dependency)

### Mesh References: Asset Paths
- Store asset path strings in MeshRendererComponent (e.g., "content/model/cube.asset")
- SceneRenderer resolves paths to StaticMesh via AssetManager and caches loaded meshes
- Avoids circular dependencies, enables future hot-reloading

### Camera: Component-Based
- CameraComponent attached to entities (not dedicated camera type)
- Supports multiple cameras, hierarchy (camera parented to objects)
- Active camera selected by "MainCamera" tag or first camera found

### Transform Updates: Automatic
- Engine calls `SceneGraph::updateTransforms()` per-frame before rendering
- Then calls `SceneGraph::updateCameras()` to compute view/projection matrices
- Dirty flags prevent redundant computation

### Renderer Integration
- SceneRenderer receives `SceneGraph const&` reference (not ownership)
- New signature: `render(CommandContext*, SceneGraph const&, clearColor)`
- Removes hardcoded cube mesh and internal camera

## New Components

### MeshRendererComponent
```cpp
struct MeshRendererComponent
{
    std::string meshAssetPath{};      // Path to .asset file
    uint32_t materialId{0};           // Material index (0 = default)
    bool castShadows{true};           // Future shadow system
    bool receiveShadows{true};
    bool enabled{true};               // Toggle rendering
};
```

### CameraComponent
```cpp
struct CameraComponent
{
    // Projection
    bool isPerspective{true};
    float fov{glm::radians(45.0f)};
    float orthoSize{10.0f};
    float nearClip{0.1f};
    float farClip{1000.0f};

    // Viewport
    uint32_t viewportWidth{1920};
    uint32_t viewportHeight{1080};

    // Computed matrices
    float4x4 viewMatrix{1.0f};
    float4x4 projectionMatrix{1.0f};
    float4x4 viewProjectionMatrix{1.0f};

    bool isDirty{true};
};
```

## ECS Extensions Required

### Problem: Multi-Component Iteration
Current `view<T>()` only iterates one component type, no access to entity IDs.

### Solution: Expose Entity Lookup
Add to `SparseSet`:
```cpp
auto getEntity(size_t denseIndex) const -> Entity
{
    return m_denseToEntity[denseIndex];
}
```

Add to `Registry`:
```cpp
template<typename T>
auto getPool() const -> SparseSet<T> const*
{
    // Return pool pointer for manual iteration
}
```

**Usage in Renderer**:
```cpp
auto const* meshPool = registry.getPool<MeshRendererComponent>();
for (size_t i = 0; i < meshPool->data().size(); ++i)
{
    auto entity = meshPool->getEntity(i);
    auto const& meshComp = meshPool->data()[i];
    if (registry.allOf<TransformComponent>(entity))
    {
        auto const& transform = registry.get<TransformComponent>(entity);
        // Render mesh with transform...
    }
}
```

## SceneGraph Extensions

### New Methods
```cpp
// Update all entity transforms (propagate hierarchy)
auto updateTransforms() -> void;

// Update all camera matrices
auto updateCameras(uint32_t viewportWidth, uint32_t viewportHeight) -> void;

// Find active camera entity
auto getActiveCamera() const -> Entity;
```

### updateTransforms Implementation
- Find all root entities (parent == NullEntity)
- Call existing `updateTransform(root, identityMatrix, false)` for each
- Requires entity iteration (uses getPool/getEntity pattern)

### updateCameras Implementation
- Iterate all CameraComponent entities
- Extract world matrix from TransformComponent
- Compute view matrix (inverse of world matrix)
- Compute projection matrix (perspective or orthographic)
- Compute viewProjection = projection * view

### getActiveCamera Implementation
- Find entity with TagComponent.tag == "MainCamera" and CameraComponent
- Fallback: Return first entity with CameraComponent
- Return NullEntity if no cameras exist

## SceneRenderer Refactor

### Changes to scene-renderer.hpp

**Remove**:
- `m_cubeMesh` (hardcoded mesh)
- `m_gameCamera` (internal camera)
- `m_useExternalCamera`, `m_hasExternalViewProj`, `m_externalViewProj` (replaced by scene cameras)
- `m_time` (no animation)
- `setUseExternalCamera()`, `setExternalViewProjection()` methods

**Add**:
```cpp
std::unordered_map<std::string, core::ref<StaticMesh>> m_meshCache;
float4x4 m_viewProjectionMatrix{1.0f};
bool m_hasActiveCamera{false};

auto getMeshForPath(std::string const& path) -> core::ref<StaticMesh>;
auto updateActiveCamera(scene::Registry const&) -> void;
auto renderMeshEntities(RenderPassEncoder*, scene::Registry const&) -> void;
```

**Change signature**:
```cpp
// Before:
auto render(CommandContext* ctx, float4 const& clearColor) -> void;

// After:
auto render(CommandContext* ctx, scene::SceneGraph const& scene, float4 const& clearColor) -> void;
```

### Rendering Algorithm

```cpp
auto SceneRenderer::render(CommandContext* ctx, SceneGraph const& scene, float4 const& clear) -> void
{
    auto const& registry = scene.getRegistry();

    // Setup render targets
    ctx->resourceBarrier(m_sceneColor, RenderTarget);
    ctx->resourceBarrier(m_sceneDepth, DepthStencil);

    auto encoder = ctx->beginRenderPass(colorTarget, depthTarget);
    encoder->setViewport(...);
    encoder->setScissor(...);

    // Extract active camera
    updateActiveCamera(registry);
    if (!m_hasActiveCamera)
    {
        encoder->end();
        return;  // No camera, skip rendering
    }

    // Render all mesh entities
    renderMeshEntities(encoder, registry);

    encoder->end();
    ctx->resourceBarrier(m_sceneColor, ShaderResource);
}
```

### getMeshForPath Implementation
```cpp
auto SceneRenderer::getMeshForPath(std::string const& path) -> core::ref<StaticMesh>
{
    // Check cache
    auto it = m_meshCache.find(path);
    if (it != m_meshCache.end())
        return it->second;

    // Load from asset
    auto mesh = m_device->createMeshFromAsset(path);
    if (mesh)
        m_meshCache[path] = mesh;

    return mesh;
}
```

### updateActiveCamera Implementation
```cpp
auto SceneRenderer::updateActiveCamera(scene::Registry const& reg) -> void
{
    auto activeCamera = scene.getActiveCamera();
    if (activeCamera == NullEntity)
    {
        m_hasActiveCamera = false;
        return;
    }

    auto const& camComp = reg.get<CameraComponent>(activeCamera);
    m_viewProjectionMatrix = camComp.viewProjectionMatrix;
    m_hasActiveCamera = true;
}
```

### renderMeshEntities Implementation
```cpp
auto SceneRenderer::renderMeshEntities(RenderPassEncoder* enc, Registry const& reg) -> void
{
    auto const* meshPool = reg.getPool<MeshRendererComponent>();
    if (!meshPool)
        return;

    for (size_t i = 0; i < meshPool->data().size(); ++i)
    {
        auto entity = meshPool->getEntity(i);
        auto const& meshComp = meshPool->data()[i];

        if (!meshComp.enabled)
            continue;

        if (!reg.allOf<TransformComponent>(entity))
            continue;

        auto const& transform = reg.get<TransformComponent>(entity);
        auto mesh = getMeshForPath(meshComp.meshAssetPath);
        if (!mesh)
            continue;

        // Set shader uniforms
        auto rootVar = m_vars->getRootVariable();
        rootVar["perFrame"]["viewProj"].setBlob(&m_viewProjectionMatrix, sizeof(float4x4));
        rootVar["perFrame"]["model"].setBlob(&transform.worldMatrix, sizeof(float4x4));

        // Draw mesh
        enc->setVao(mesh->getVAO());
        enc->bindPipeline(m_pipeline, m_vars);

        for (size_t s = 0; s < mesh->getSubmeshCount(); ++s)
        {
            auto const& submesh = mesh->getSubmesh(s);
            enc->drawIndexed(submesh.indexCount, submesh.indexOffset, 0);
        }
    }
}
```

## Engine Integration

### Add to Engine class (engine.hpp)
```cpp
#include "scene/scene.hpp"

class Engine
{
    // ...
    std::unique_ptr<scene::SceneGraph> m_sceneGraph;

public:
    auto getSceneGraph() -> scene::SceneGraph* { return m_sceneGraph.get(); }
};
```

### Update Engine::init() (engine.cpp)
```cpp
auto Engine::init(EngineConfig const& config) -> void
{
    // ... existing initialization ...

    // Create scene graph
    m_sceneGraph = std::make_unique<scene::SceneGraph>();

    // ... rest of initialization ...
}
```

### Update Engine::renderFrame() (engine.cpp)
```cpp
auto Engine::renderFrame(float deltaTime) -> void
{
    // ... swapchain/viewport setup ...

    // User update hook
    if (m_hooks.onUpdate)
        m_hooks.onUpdate(deltaTime);

    // NEW: Update scene systems
    if (m_sceneGraph)
    {
        m_sceneGraph->updateTransforms();
        m_sceneGraph->updateCameras(m_width, m_height);
    }

    // Render scene
    auto const clearColor = m_config.clearColor;

    if (m_hooks.onRender)
    {
        m_hooks.onRender(m_context, renderTargetView.get());
    }
    else if (m_renderer && m_sceneGraph)
    {
        m_renderer->render(m_context, *m_sceneGraph, clearColor);
    }
    else
    {
        // Fallback clear
        m_context->clearRtv(renderTargetView.get(), clearColor);
    }

    // ... UI / composite / present ...
}
```

### Update runtime manifest (if needed)
Check `engine/runtime/manifest.txt`:
```ini
[dependencies]
public = core, graphics, ui, scene
```

## File Changes Summary

### Modified Files (8 files)

1. **engine/scene/source/scene/components.hpp**
   - Add MeshRendererComponent struct
   - Add CameraComponent struct

2. **engine/scene/source/scene/ecs-core.hpp**
   - Add `SparseSet::getEntity(size_t) const -> Entity`
   - Add `Registry::getPool<T>() const -> SparseSet<T> const*`

3. **engine/scene/source/scene/scene-graph.hpp**
   - Add `updateTransforms()` declaration
   - Add `updateCameras(uint32_t, uint32_t)` declaration
   - Add `getActiveCamera() const -> Entity` declaration

4. **engine/scene/source/scene/scene-graph.cpp**
   - Implement updateTransforms() (iterate roots, call existing updateTransform)
   - Implement updateCameras() (compute view/proj matrices for all cameras)
   - Implement getActiveCamera() (find "MainCamera" tag or first camera)

5. **engine/graphics/source/graphics/renderer/scene-renderer.hpp**
   - Change render() signature to accept SceneGraph const&
   - Remove external camera members/methods
   - Add mesh cache, camera state, and private rendering methods

6. **engine/graphics/source/graphics/renderer/scene-renderer.cpp**
   - Remove hardcoded cube mesh loading from constructor
   - Implement getMeshForPath() with caching
   - Implement updateActiveCamera() to extract camera from scene
   - Implement renderMeshEntities() to iterate and draw mesh components
   - Rewrite render() to use scene graph

7. **engine/runtime/source/runtime/engine.hpp**
   - Add `#include "scene/scene.hpp"`
   - Add `std::unique_ptr<scene::SceneGraph> m_sceneGraph` member
   - Add `getSceneGraph()` accessor

8. **engine/runtime/source/runtime/engine.cpp**
   - Create SceneGraph in init()
   - Call updateTransforms() and updateCameras() in renderFrame()
   - Pass SceneGraph to renderer->render()

## Verification Plan

### Test 1: Empty Scene
- Build and run with empty scene graph
- Expected: Clear color displayed, no crashes

### Test 2: Single Entity
Add to Engine::init() after creating scene graph:
```cpp
auto camera = m_sceneGraph->createEntity("MainCamera");
m_sceneGraph->getRegistry().emplace<scene::CameraComponent>(camera);
auto& camTransform = m_sceneGraph->getRegistry().get<scene::TransformComponent>(camera);
camTransform.localPosition = {0.0f, 3.0f, 10.0f};
camTransform.isDirty = true;

auto cube = m_sceneGraph->createEntity("Cube");
auto& meshRenderer = m_sceneGraph->getRegistry().emplace<scene::MeshRendererComponent>(cube);
meshRenderer.meshAssetPath = "E:/github/April2/content/model/cube.asset";
```
- Expected: Single cube at origin, viewed from (0, 3, 10)

### Test 3: Multiple Entities
Add second cube at different position:
```cpp
auto cube2 = m_sceneGraph->createEntity("Cube2");
auto& meshRenderer2 = m_sceneGraph->getRegistry().emplace<scene::MeshRendererComponent>(cube2);
meshRenderer2.meshAssetPath = "E:/github/April2/content/model/cube.asset";
auto& transform2 = m_sceneGraph->getRegistry().get<scene::TransformComponent>(cube2);
transform2.localPosition = {3.0f, 0.0f, 0.0f};
```
- Expected: Two cubes visible at different positions

### Test 4: Hierarchy
```cpp
m_sceneGraph->setParent(cube2, cube);
auto& cubeTransform = m_sceneGraph->getRegistry().get<scene::TransformComponent>(cube);
cubeTransform.localRotation.y = glm::radians(45.0f);
cubeTransform.isDirty = true;
```
- Expected: Child cube rotates with parent

### Test 5: Camera Switching
Create second camera and switch MainCamera tag to verify active camera selection.
- Expected: View changes to new camera perspective

## Implementation Sequence

1. **ECS Extensions** (30 min)
   - Add getEntity to SparseSet
   - Add getPool to Registry
   - Verify compilation

2. **Scene Components** (15 min)
   - Add MeshRendererComponent
   - Add CameraComponent
   - Verify compilation

3. **SceneGraph Systems** (1 hour)
   - Implement updateTransforms
   - Implement updateCameras
   - Implement getActiveCamera
   - Add basic logging for debugging

4. **SceneRenderer Refactor** (2 hours)
   - Update header (new signature, remove old, add new members)
   - Implement getMeshForPath
   - Implement updateActiveCamera
   - Implement renderMeshEntities
   - Update render() method
   - Remove hardcoded cube from constructor

5. **Engine Integration** (30 min)
   - Add SceneGraph member to Engine
   - Create in init()
   - Update renderFrame() to call scene updates
   - Pass scene to renderer
   - Update manifest dependencies if needed

6. **Testing** (1 hour)
   - Test 1: Empty scene
   - Test 2: Single cube
   - Test 3: Multiple cubes
   - Test 4: Hierarchy
   - Test 5: Camera switching
   - Fix bugs as found

**Total Time: ~5 hours**

## Critical Paths

The critical dependencies for implementation:
1. ECS extensions → SceneGraph systems (needs entity iteration)
2. Scene components → Renderer (renderer queries components)
3. SceneGraph systems → Engine integration (engine calls scene updates)
4. All above → Testing

Files must be edited in order:
1. ecs-core.hpp (foundational ECS changes)
2. components.hpp (data structures)
3. scene-graph.hpp/cpp (systems)
4. scene-renderer.hpp/cpp (rendering)
5. engine.hpp/cpp (integration)


## Revisions & Fixes (Critical Architecture Updates)
Based on architectural review, the following changes supersede the original plan above to ensure performance and consistency with the UUID-based Asset System.

1. Switch from String Paths to UUIDs
To align with the Stage 1 AssetManager system and avoid costly string hashing per frame, we will use core::UUID references.

Updated Component (components.hpp):

C++
#include <core/tools/uuid.hpp>

struct MeshRendererComponent
{
    core::UUID meshAssetHandle{};      // CHANGED: Use UUID instead of std::string path
    core::UUID materialAssetHandle{};  // Placeholder for future material system
    bool castShadows{true};
    bool receiveShadows{true};
    bool enabled{true};
};
2. Refined Renderer Resource Management
The SceneRenderer should not be responsible for loading files from disk. It should request data from AssetManager and manage the CPU->GPU upload and caching.

Updated SceneRenderer Members (scene-renderer.hpp):

C++
// Dependency on AssetManager
asset::AssetManager* m_assetManager{nullptr};

// Cache maps UUID to GPU resource
std::unordered_map<core::UUID, core::ref<StaticMesh>> m_gpuMeshCache;

// Update init signature to receive AssetManager
auto init(rhi::Device* device, asset::AssetManager* assetMgr) -> void;

// Helper to resolve GPU resources
auto getGPUMesh(core::UUID handle) -> core::ref<StaticMesh>;
Updated Implementation Logic (scene-renderer.cpp):

C++
auto SceneRenderer::getGPUMesh(core::UUID handle) -> core::ref<StaticMesh>
{
    // 1. Check GPU cache
    if (m_gpuMeshCache.contains(handle))
        return m_gpuMeshCache.at(handle);

    // 2. Resolve Asset via AssetManager (Metadata lookup)
    auto asset = m_assetManager->getAsset<asset::StaticMeshAsset>(handle);
    if (!asset) return nullptr; // Asset not found or not loaded

    // 3. Request Binary Data (Load from DDC if necessary)
    std::vector<std::byte> blob;
    auto payload = m_assetManager->getMeshData(*asset, blob);

    if (payload.vertexData.empty()) return nullptr;

    // 4. Create GPU Resource
    // (Assuming createStaticMesh helper exists in Device or locally)
    auto mesh = m_device->createStaticMesh(payload);

    // 5. Cache and Return
    m_gpuMeshCache[handle] = mesh;
    return mesh;
}
3. Transform Logic Correction
In SceneGraph::updateTransforms, ensure the dirty flag propagates correctly. A child must update if either it is dirty OR its parent is dirty.

Logic Check:

C++
// Inside recursive update function
bool effectiveDirty = transform.isDirty || parentIsDirty;

if (effectiveDirty) {
    // Recalculate world matrix...
    // ...
}
// Pass effectiveDirty to children
4. Engine Integration Adjustment
Ensure AssetManager is passed to the renderer during initialization.

Updated engine.cpp:

C++
auto Engine::init(EngineConfig const& config) -> void
{
    // ... AssetManager creation ...
    // ... Renderer creation ...

    // Inject dependency
    m_renderer->init(m_device.get(), m_assetManager.get());
}
