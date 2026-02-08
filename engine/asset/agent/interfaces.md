# Asset Public Interfaces

## Purpose
CPU-side asset import and loading for .asset metadata and compiled texture/mesh blobs.

## Usage Patterns
- Create one `AssetManager` and keep it alive while assets are in use.
- Use `importAsset()` to generate/update .asset files from source files.
- Use `getAsset<T>()`/`loadAsset<T>()` for typed metadata access.
- Blob payloads reference memory you own (e.g., `outBlob` in AssetManager calls).

## Public Header Index
- `asset/asset-manager.hpp` — CPU-side asset import/load/cache manager with registry and DDC integration.
- `asset/blob-header.hpp` — Binary layout definitions for compiled texture/mesh blobs and payload views.
- `asset/material-asset.hpp` — Material asset schema (PBR parameters + texture references) with JSON serialization.
- `asset/static-mesh-asset.hpp` — Static mesh asset metadata, import settings, and material slot mapping.
- `asset/texture-asset.hpp` — Texture asset import settings and JSON serialization.

## Header Details

### asset/asset-manager.hpp
Location: `engine/asset/source/asset/asset-manager.hpp`
Include: `#include <asset/asset-manager.hpp>`

Purpose: CPU-side asset import/load/cache manager with registry and DDC integration.

Key Types: `AssetManager`, `ImportPolicy`, `ImportConfig`
Key APIs: `AssetManager::importAsset(...)`, `AssetManager::getAsset<T>(UUID)`, `AssetManager::loadAsset<T>(path)`, `AssetManager::getTextureData(...)`, `AssetManager::getMeshData(...)`, `AssetManager::scanDirectory(...)`

Usage Notes:
- Construct once with `assetRoot` and `cacheRoot` and keep it alive for asset lifetime.
- Use `importAsset()` to create/update `.asset` metadata files from source assets.
- Use `getTextureData()`/`getMeshData()` with an output blob that owns the payload memory.

Used By: `editor`, `graphics`, `runtime`, `scene`

### asset/blob-header.hpp
Location: `engine/asset/source/asset/blob-header.hpp`
Include: `#include <asset/blob-header.hpp>`

Purpose: Binary layout definitions for compiled texture/mesh blobs and payload views.

Key Types: `PixelFormat`, `TextureHeader`, `TexturePayload`, `VertexFlags`, `Submesh`, `MeshHeader`, `MeshPayload`
Key APIs: `TextureHeader::isValid()`, `MeshHeader::isValid()`

Usage Notes:
- Use `isValid()` on headers before consuming payloads.
- Payload spans reference external blob memory; keep the blob alive.

Used By: `graphics`

### asset/material-asset.hpp
Location: `engine/asset/source/asset/material-asset.hpp`
Include: `#include <asset/material-asset.hpp>`

Purpose: Material asset schema (PBR parameters + texture references) with JSON serialization.

Key Types: `MaterialParameters`, `TextureReference`, `MaterialTextures`, `MaterialAsset`
Key APIs: `MaterialAsset::serializeJson(...)`, `MaterialAsset::deserializeJson(...)`, `to_json/from_json for MaterialParameters/TextureReference/MaterialTextures`

Usage Notes:
- Populate `MaterialParameters`/`MaterialTextures` and serialize to `.asset` JSON.
- Used by material importers and runtime material creation.

Used By: `graphics`, `scene`

### asset/static-mesh-asset.hpp
Location: `engine/asset/source/asset/static-mesh-asset.hpp`
Include: `#include <asset/static-mesh-asset.hpp>`

Purpose: Static mesh asset metadata, import settings, and material slot mapping.

Key Types: `MaterialSlot`, `MeshImportSettings`, `StaticMeshAsset`
Key APIs: `MeshImportSettings`, `StaticMeshAsset::serializeJson(...)`, `StaticMeshAsset::deserializeJson(...)`

Usage Notes:
- Material slots map submeshes to material references.
- Import settings drive mesh processing (tangents, optimization, scale).

Used By: `scene`

### asset/texture-asset.hpp
Location: `engine/asset/source/asset/texture-asset.hpp`
Include: `#include <asset/texture-asset.hpp>`

Purpose: Texture asset import settings and JSON serialization.

Key Types: `TextureImportSettings`, `TextureAsset`
Key APIs: `TextureImportSettings`, `TextureAsset::serializeJson(...)`, `TextureAsset::deserializeJson(...)`

Usage Notes:
- Use settings to control sRGB, mip generation, compression, and brightness.

Used By: `scene`
