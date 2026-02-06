# C++ Game Engine Asset + Importer + DDC Architecture (Detailed Implementation Guide)

This document describes a practical, step-by-step architecture for building an **asset pipeline** in a C++ game engine where:

- **Assets** are stable, referenceable “shell” objects (small metadata + references).
- **Derived (runtime) data** is stored in a **DDC (Derived Data Cache)**.
- The pipeline supports **incremental builds**, **deterministic outputs**, **dependency tracking**, **multi-platform targets**, and clean separation between **Editor** and **Runtime**.

The design applies to common asset types such as **Texture**, **Material**, and **Mesh**.

---

## Table of Contents

1. [Core Concepts](#core-concepts)
2. [High-Level Architecture](#high-level-architecture)
3. [Project Layout](#project-layout)
4. [Identity & Referencing (GUID + AssetRef)](#identity--referencing-guid--assetref)
5. [Asset Shell Files (.asset) & Optional Meta](#asset-shell-files-asset--optional-meta)
6. [Asset Registry (AssetDB)](#asset-registry-assetdb)
7. [DDC Design](#ddc-design)
8. [Key & Fingerprint System](#key--fingerprint-system)
9. [Importer Framework](#importer-framework)
10. [Dependency Graph & Change Propagation](#dependency-graph--change-propagation)
11. [Incremental Import Scheduler](#incremental-import-scheduler)
12. [Implementing Texture](#implementing-texture)
13. [Implementing Material](#implementing-material)
14. [Implementing Mesh](#implementing-mesh)
15. [Sub-Assets](#sub-assets)
16. [Runtime Loading (Dev vs Shipping)](#runtime-loading-dev-vs-shipping)
17. [Packing & Catalog](#packing--catalog)
18. [Determinism, Concurrency, and Safety](#determinism-concurrency-and-safety)
19. [Diagnostics & Last-Known-Good](#diagnostics--last-known-good)
20. [Testing & Validation](#testing--validation)
21. [Recommended Implementation Order](#recommended-implementation-order)

---

## Core Concepts

### Source
Raw files on disk:
- `*.png`, `*.jpg`, `*.tga`
- `*.fbx`, `*.gltf`, `*.obj`
- `*.hlsl`, `*.glsl`, `*.shadergraph`
- etc.

### Asset (Shell)
A stable, referenceable object:
- Identified by **GUID**
- Stores **type**, **settings**, **references to other assets**
- Does **not** contain large runtime buffers (those live in DDC)

### Derived Data (Artifacts)
Binary data produced by importers:
- compressed texture blocks
- mesh vertex/index buffers, cluster data
- compiled shader blobs, material binding layouts

Stored in **DDC**, referenced by **DDC keys**.

### Importer
A deterministic transformation:
- Input: `Source + Settings + TargetProfile + ToolchainVersion + Dependencies`
- Output: `Derived data in DDC + dependency declarations + sub-asset descriptors`

---

## High-Level Architecture

         +-------------------+
         |   Source Files     |
         |  (content/...)      |
         +---------+---------+
                   |
                   v
         +-------------------+
         | Asset Shell Files  |
         |   (*.asset)        |
         | GUID + refs + cfg  |
         +---------+---------+
                   |
                   v
         +-------------------+         +--------------------+
         | Import Scheduler   |<------->| Asset Registry/DB   |
         | (incremental)      |         | GUID->path, deps,   |
         +---------+---------+          | ddc keys, state     |
                   |
                   v
         +-------------------+         +--------------------+
         | Importers         |-------->| DDC (Derived Cache)|
         | (Texture/Mesh/...)|  Put()  | key -> blobs       |
         +-------------------+         +--------------------+
Runtime:
AssetRef(GUID, subId) -> Catalog/Registry -> DDCKey -> load blob -> create resource

## Usage (Current Engine)
Basic usage patterns for the current implementation:

Import a source file (creates .asset files):
```cpp
auto manager = april::asset::AssetManager{};
auto asset = manager.importAsset("content/mesh/hero.gltf");
```

Import with configuration (Unreal/Blender-style toggles + settings override):
```cpp
auto config = april::asset::AssetManager::ImportConfig{};
config.policy = april::asset::AssetManager::ImportPolicy::ReimportIfSourceChanged;
config.importMaterials = true;
config.importTextures = false;
config.reuseExistingAssets = true;
config.overrideMeshSettings = true;
config.meshSettings.optimize = true;
config.meshSettings.generateTangents = true;
config.meshSettings.scale = 1.0f;

auto mesh = manager.importAsset("content/mesh/hero.gltf", config);
```

Load an asset by path and fetch cooked data:
```cpp
auto mesh = manager.loadAsset<april::asset::StaticMeshAsset>("content/mesh/hero.gltf.asset");
if (mesh)
{
  std::vector<std::byte> blob;
  auto payload = manager.getMeshData(*mesh, blob);
}
```

Load an asset by UUID:
```cpp
auto texture = manager.getAsset<april::asset::TextureAsset>(someGuid);
```

Notes:
- `importAsset` resolves the importer by file extension and creates one or more `.asset` files.
- `getTextureData`/`getMeshData` call `ensureImported`, which triggers a cook only when the DDC key is missing.
- Materials are authored assets; they are cooked from their `.asset` data rather than a raw source file.


---

## Project Layout

Recommended top-level layout:

- `content/`
  - Source files and `.asset` shells
- `content/library/` (generated; safe to delete)
  - `content/library/ddc/` (local derived data cache)
  - `content/library/asset-db/` (registry/indices)
- `Build/` (final build outputs)
  - `Build/<platform>/Content.pack`
  - `Build/<platform>/Catalog.bin` (or `.json`)
- `ProjectSettings/`
  - global defaults, platform profiles, toolchain hashes, etc.

**Rule:** Deleting `Library/` must not break the project permanently; it just triggers rebuild.

---

## Identity & Referencing (GUID + AssetRef)

### GUID
- Must be stable across rename/move
- Must be unique per asset

Use 128-bit GUID.

Generate once, store in .asset (or .meta).

### AssetRef
All cross-asset references use AssetRef (never file paths).

```cpp
struct AssetRef
{
  Guid guid;
  uint32_t subId = 0; // 0 = main asset; >0 = sub-asset
};
```
Do not store “path” for references inside assets. Paths are for humans; GUIDs are for stability.

### Asset Shell Files (.asset) & Optional Meta
Why .asset?
Some asset types have no single source file (e.g., Material authored in editor). A .asset file is a natural identity container.

Recommended .asset schema (JSON for early dev)
Example: Assets/Textures/wood-texture.asset

```json
{
  "guid": "b2c5f3d2-a7b4-4c1e-a0d5-b9c8a91f6c0a",
  "type": "Texture",
  "source_path": "Assets/Textures/wood.png",
  "importer": "TextureImporter@v1",
  "settings": {
    "srgb": true,
    "mipmaps": true,
    "maxSize": 2048,
    "compression": "BC7"
  },
  "refs": [],
  "tags": ["Environment"]
}
```
Example: Assets/Materials/hero-mat.asset (no single source file)

```json
{
  "guid": "84c10d11-3e47-4a05-b5e7-33e1b2a5d9a2",
  "type": "Material",
  "importer": "MaterialImporter@v1",
  "settings": {
    "shader": { "guid": "11111111-2222-3333-4444-555555555555", "subId": 0 },
    "params": {
      "BaseColorTex": { "guid": "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee", "subId": 0 },
      "Roughness": 0.8
    }
  },
  "refs": [
    { "guid": "11111111-2222-3333-4444-555555555555", "subId": 0 },
    { "guid": "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee", "subId": 0 }
  ]
}
```
Importer field
`importer` stores the importer chain string:
- Root asset: `ImporterId@vN`
- Sub-asset via another importer: `Parent@vA|Child@vB`
Meta files (optional)
If you prefer source-centric: wood.png.meta can store GUID and importer settings.
But .asset is often simpler long-term, especially for non-source-authored assets.

## Asset Registry (AssetDB)
The registry is the engine’s “truth center” for:

GUID → .asset path

GUID → type

last import fingerprint per target

DDC keys per target

dependencies (strong/weak)

reverse dependencies (who depends on me)

Minimal Registry Records
```cpp
enum class DepKind : uint8_t { Strong, Weak };

struct Dependency
{
  DepKind kind;
  Guid guid;              // asset dependency
  // optionally external dependency:
  // std::string filePath;
  // std::string fileContentHash;
};

struct TargetProfile
{
  std::string platform;   // "Win64", "Android", ...
  std::string gpuFormat;  // "BC7", "ASTC", ...
  std::string quality;    // "Debug", "Release", ...
};

struct AssetRecord
{
  Guid guid;
  std::string assetPath;  // path to .asset
  std::string type;       // "Texture", "Material", "Mesh", ...

  // Dependencies discovered during import:
  std::vector<Dependency> deps;

  // Per target:
  std::unordered_map<std::string /*targetId*/, std::string /*fingerprint*/> lastFingerprint;
  std::unordered_map<std::string /*targetId*/, std::vector<std::string /*ddcKey*/>> ddcKeys;
  std::string lastSourceHash; // source content hash, not per target

  // State:
  bool lastImportFailed = false;
  std::string lastErrorSummary;
};
```
targetId can be a canonical string like: Win64|BC7|Release.

Storage
Start simple:

Library/AssetDB/registry.json (or binary)

Build indices at startup by scanning Assets/**/*.asset

Later upgrade to SQLite if needed.

## DDC Design
DDC stores derived data by key:

key -> value bytes

DDC Interface
```cpp
struct DdcValue
{
  std::vector<uint8_t> bytes;
  std::string contentHash; // optional integrity check
};

class IDdc
{
public:
  virtual ~IDdc() = default;
  virtual bool get(const std::string& key, DdcValue& out) = 0;
  virtual void put(const std::string& key, const DdcValue& value) = 0;
  virtual bool exists(const std::string& key) = 0;
};
```
Local Disk Implementation
Root: content/library/DDC/

Use sharded directories by hash prefix:

content/library/DDC/ab/cd/<hash>.bin

Do not use raw key as filename (too long, unsafe chars).

hash = SHA1(key) or BLAKE3(key).

Example:

path = Library/DDC/<h[0..1]>/<h[2..3]>/<h>.bin

Atomic writes
Write to temp file + rename:

tmp = <path>.tmp.<pid>.<thread>

write fully, flush

rename tmp → final

## Key & Fingerprint System
Requirements
A DDC key must change when any of these change:

source content

settings

importer version

toolchain / library versions

target platform/profile

relevant dependencies

Canonical Hashing
Source hash

Prefer content hash for correctness

Optionally two-phase:

quick check (mtime,size)

if changed, compute content hash

Settings hash

Canonicalize JSON:

stable key ordering

stable float formatting

remove whitespace

Then hash

Toolchain hash

includes versions of:

texture compressor

mesh optimizer

shader compiler

also include your own “DDC schema version” constant

Deps hash

combine dependency fingerprints (strong deps only)

Fingerprint input struct
```cpp
struct FingerprintInput
{
  Guid assetGuid;
  std::string importerId;
  int importerVersion;
  std::string toolchainHash;
  std::string sourceContentHash;  // empty for assets without sources
  std::string settingsHash;
  std::string depsHash;
  TargetProfile target;
};
```
DDC key format
Readable keys help debugging:

<type>|<guid>|imp=<importerId>@v<ver>|tgt=<platform>,<gpu>,<q>|S=<settings>|C=<source>|D=<deps>|T=<toolchain>
Example:

```text
TX|b2c5...|imp=TextureImporter@v3|tgt=Win64,BC7,Release|S=91ab..|C=0f12..|D=0000..|T=2cdd..
```
DDC Key vs Fingerprint
Fingerprint: stored in registry to decide if import is needed

DDC key: used to retrieve derived blobs

In practice you can use the same string for both, or store fingerprint as a shorter hash and key as the full descriptor.

## Importer Framework
Two-phase import: **source import** (build assets) + **cook** (build DDC).

Source import context/result
```cpp
struct ImportSourceContext
{
  std::filesystem::path sourcePath;
  std::string importerChain;

  std::function<std::shared_ptr<Asset>(std::filesystem::path const&, AssetType)> findAssetBySource;
  std::function<std::shared_ptr<Asset>(std::filesystem::path const&)> importSubAsset;
};

struct ImportSourceResult
{
  std::shared_ptr<Asset> primaryAsset;
  std::vector<std::shared_ptr<Asset>> assets;
  std::vector<std::string> warnings;
  std::vector<std::string> errors;
};
```

Cook context/result
```cpp
struct ImportCookContext
{
  Asset const& asset;
  std::string assetPath;
  std::string sourcePath;
  TargetProfile target;
  IDdc& ddc;
  DepRecorder& deps;
  bool forceReimport = false;
};

struct ImportCookResult
{
  std::vector<std::string> producedKeys;
  std::vector<std::string> warnings;
  std::vector<std::string> errors;
};
```

Importer interface
```cpp
class IImporter
{
public:
  virtual ~IImporter() = default;
  virtual std::string_view id() const = 0;
  virtual int version() const = 0;
  virtual bool supportsExtension(std::string_view ext) const = 0;
  virtual AssetType primaryType() const = 0;

  virtual ImportSourceResult import(ImportSourceContext const& ctx) = 0;
  virtual ImportCookResult cook(ImportCookContext const& ctx) = 0;
};
```
Dependency recorder
```cpp
struct DepRecorder
{
  std::vector<Dependency> deps;
  void AddStrong(Guid g) { deps.push_back({DepKind::Strong, g}); }
  void AddWeak(Guid g)   { deps.push_back({DepKind::Weak, g}); }
};
```
## Dependency Graph & Change Propagation
Strong vs Weak
Strong: changes must trigger re-import of dependent assets

Weak: changes may affect preview only; usually do not force rebuild

Reverse dependency index
Maintain a map:

depGuid -> vector<whoDependsOnMe>

This enables fast propagation:

When A changes, mark all dependents(A) as dirty.

Dirtiness propagation algorithm
When an asset changes or is re-imported:

Update its dependency list in registry.

Update reverse indices.

Mark dependents dirty (BFS/DFS), but only through strong edges.

## Incremental Import Scheduler
States
Each asset per target can be:

UpToDate

NeedsImport

Importing

Failed (with last-known-good kept)

Minimal scheduler steps
For each asset you care about:

Load .asset

Compute fingerprint

Compare to registry lastFingerprint[target]

If different:

NeedsImport

enqueue import job

After import:

If success:

store new fingerprint

store produced DDC keys

store deps

If fail:

mark failed + store diagnostics

keep old keys in registry for last-known-good behavior

Concurrency (later)
Start single-threaded for correctness.
When stable:

parallelize imports with a job system

ensure DDC writes are atomic

ensure registry updates are synchronized

## Implementing Texture
Texture Asset Shell
wood.texture.asset references source image.

Important settings:

srgb

mipmaps

maxSize

compression

normalMap (affects channel swizzling, mip gen)

platform overrides (optional)

Texture derived data format (example)
Define a binary layout for DDC value:

```cpp
struct TextureCookedHeader
{
  uint32_t magic;        // 'TXCD'
  uint16_t version;      // schema version
  uint16_t format;       // enum (BC7, ASTC, RGBA8...)
  uint16_t width;
  uint16_t height;
  uint16_t mipCount;
  uint32_t dataSize;
  // followed by mip offsets table + blob
}
```
TextureImporter steps
Read source pixels

Apply settings:

resize to maxSize

generate mipmaps

compress based on target.gpuFormat / settings.compression

Serialize to TextureCooked binary

Compute DDC key from fingerprint inputs

Put into DDC

Pseudocode
```cpp
auto TextureImporter::Import(const ImportContext& ctx) -> ImportResult {
  DepRecorder deps; // textures usually have no asset deps
  ctx.deps->deps.clear();

  auto sourceHash = HashFileContents(ctx.sourcePath);
  auto settingsHash = HashCanonicalJson(ctx.settings);
  auto depsHash = "0000"; // none

  auto key = BuildDdcKey({
    ctx.guid, Id(), Version(), ToolchainHash_Texture(),
    sourceHash, settingsHash, depsHash, ctx.target
  });

  if (ctx.ddc->Exists(key))
  {
    return { .producedKeys = { key } };
  }

  Image img = LoadImage(ctx.sourcePath);
  img = ApplyResize(img, ctx.settings["maxSize"]);
  auto mips = GenerateMips(img, ctx.settings["mipmaps"]);
  auto compressed = Compress(mips, ctx.target, ctx.settings);

  DdcValue val;
  val.bytes = SerializeTextureCooked(compressed, ctx.target);

  ctx.ddc->Put(key, val);

  // record results
  return { .producedKeys = { key } };
}
```
## Implementing Material
Materials are usually not “loaded from PNG/FBX” but from authored graphs or parameters. They depend on:

Shader asset (or shader source)

Texture assets

Possibly other assets (e.g., LUT, function libs)

Material shell
references: shader + textures

settings: parameter values, feature switches, static defines

Material derived data (DDC)
Material “cooked” output often contains:

uniform buffer layout / offsets

resource binding table (which textures go to which slots)

selected shader permutations

pre-baked constants

MaterialImporter responsibilities
Gather strong dependencies:

shader guid

texture guids

Create a stable representation of:

static defines

parameter defaults

binding layout

Optionally compile needed shader permutations (prefer shader compilation as its own asset/step, also using DDC)

Serialize MaterialCooked to DDC

MaterialCooked example layout
```cpp
struct MaterialCookedHeader
{
  uint32_t magic; // 'MTCD'
  uint16_t version;
  uint16_t resourceCount;
  uint16_t uniformDataSize;
  // followed by:
  // ResourceBinding[resourceCount]
  // UniformData[uniformDataSize]
  // ShaderPermutationKeys[]
};
```
Dependency correctness
If any referenced texture changes, the material must be recompiled if:

binding layout changes

shader static defines depend on texture presence/features

you embed texture metadata into material cooked output

If your material cooked output only stores GUIDs/slots and runtime resolves textures separately, then texture changes might not require material rebuild. Decide explicitly and encode in dependency rules.

Recommended: treat texture references as strong dependencies at first (safe), then relax later if needed.

## Implementing Mesh
Mesh importers typically:

parse source (FBX/GLTF/OBJ/custom)

build LODs

optimize vertex cache / meshlets

build adjacency/tangents

compute bounds

optionally generate collision data

Mesh shell
Mesh shell stores:

source path

import settings (scale, axis, LOD policy)

material slot references (AssetRef list)

optional sub-asset list (multiple meshes/clips from one source)

Example snippet:

```json
{
  "type": "Mesh",
  "source_path": "Assets/Meshes/hero.fbx",
  "settings": {
    "scale": 1.0,
    "generateLODs": true,
    "lodCount": 3,
    "computeTangents": true
  },
  "refs": [
    { "guid": "<material-slot0-guid>", "subId": 0 },
    { "guid": "<material-slot1-guid>", "subId": 0 }
  ]
}
```
Mesh derived data (DDC)
A single MeshCooked blob might contain:

header + version

bounds (AABB/sphere)

LOD table

for each LOD:

vertex buffer

index buffer

section/submesh table

optional: meshlet clusters / BVH / collision

MeshImporter dependencies
Geometry data usually does not depend on materials. So:

MeshImporter may not need to treat materials as dependencies for the cooked geometry blob.

But the mesh asset shell still references materials for runtime rendering.

Recommended approach:

Keep mesh geometry cooking independent from materials

Materials only affect runtime binding, not cooked geometry

Store materials refs in .asset, not in DDC geometry blob

## Sub-Assets
Sub-assets occur when one source produces multiple logical assets:

FBX → multiple meshes, skeleton, animation clips

Texture atlas → multiple sprites

Audio bank → multiple clips

Stable subId strategy
Do not use indices (unstable). Use hashed identifiers based on stable names/paths:

subId = Hash32( type + ":" + stablePathOrName )
Examples:

mesh node path: Root/Body/Head

animation clip name: RunForward

Registry representation
Store discovered sub-assets in registry record:

(parentGuid -> list of {subId, name, type})

Runtime references sub-assets via (guid, subId).

Importer chain rules
- Each asset stores an importer chain string: `ImporterId@vN`.
- If a sub-asset is imported via another importer, append: `Parent@vA|Child@vB`.
- If a sub-asset is produced directly by the same importer, keep the parent chain only.

## Runtime Loading (Dev vs Shipping)
Dev mode (Editor / local runs)
runtime can load from local DDC

fast iteration

Flow:

AssetRef → registry → DDCKey

DDC.Get(key) → bytes

Deserialize → create GPU/CPU resource

Shipping mode (Packfile)
Shipping builds should not depend on Library/DDC.

Flow:

Build step gathers required DDC blobs

Packs them into Content.pack

Writes Catalog mapping AssetRef -> pack offset/size/hash

Runtime loads from pack via catalog

## Packing & Catalog
Packfile format (simple)
A sequential blob file:

header

chunk table (optional)

payload blobs (DDC values)

Catalog
Catalog maps AssetRef (GUID + subId) and/or DDCKeyHash to pack locations.

Two common options:

Option A: Catalog by AssetRef

AssetRef -> list<DDCKeyHash> -> offset/size
Pros: runtime requests by AssetRef naturally
Cons: you must ensure 1:1 mapping from AssetRef to the relevant key(s)

Option B: Catalog by DDCKeyHash

DDCKeyHash -> offset/size
Pros: simplest and matches DDC storage
Cons: runtime must know the key (registry must ship or be embedded)

Recommended hybrid:

Shipping: AssetRef -> DDCKeyHash

Catalog: DDCKeyHash -> offset/size

## Determinism, Concurrency, and Safety
Determinism rules
Importers must not depend on:

current time

random seeds (unless fixed)

thread scheduling order

OS locale / floating formatting inconsistencies

Ensure stable serialization:

canonical JSON hashing

stable chunk ordering

stable float rounding rules

Concurrency rules
DDC writes must be atomic (temp + rename)

Import jobs must not write to shared output paths directly

Registry updates must be locked or funneled through a single-thread commit phase

## Diagnostics & Last-Known-Good
Last-known-good
If an import fails:

keep old ddcKeys[target] in registry

mark asset as Failed

editor shows diagnostics but can still run with previous derived data

Diagnostics data
Store in registry:

last error summary

list of warnings/errors

timestamp (optional)

importer id/version used

## Testing & Validation
Unit tests
Canonical JSON hashing is stable across key order

DDC key changes when any input changes

DDC key remains same when inputs same

Integration tests
Change a texture source: only that texture reimports

Change a texture referenced by a material: material reimports (if strong dep)

Change unrelated file: no reimport of unrelated assets

Determinism tests
Import same assets on two machines:

derived data hashes should match

DDC keys should match

## Recommended Implementation Order
A minimal, low-risk plan:

Asset shell format + GUID + AssetRef

Local DDC (Get/Put/Exists)

Asset Registry (scan Assets/, store records)

KeyBuilder + Fingerprint (source/settings/toolchain/target)

TextureImporter end-to-end

Incremental scheduler

Dependency graph + propagation

MaterialImporter + shader/DDC integration

MeshImporter (geometry cooked into DDC)

Runtime loader (Dev) reading from DDC

Packfile + Catalog

Runtime loader (Shipping) reading from pack

## Appendix: Canonical JSON Hashing Notes
To avoid false rebuilds, canonicalize settings:

sort object keys lexicographically

normalize numeric formatting:

fixed precision for floats (e.g., 6 decimals)

remove whitespace

ensure consistent boolean/null literals

Then hash the canonical string.

## Appendix: Target Profiles
Use a small struct to represent build targets:

```cpp
struct TargetProfile
{
  std::string platform;   // Win64/Android/PS5
  std::string gpuFormat;  // BC7/ASTC
  std::string quality;    // Debug/Release/Low/High

  std::string ToId() const {
    return platform + "|" + gpuFormat + "|" + quality;
  }
};
```
## Appendix: Practical Tips
Keep .asset human-readable early (JSON). Convert to binary later if needed.

Start single-threaded; go parallel after correctness.

Always include importer version in key to invalidate old data safely.

Prefer content hash for reproducibility; use quick checks only as optimization.

Treat dependencies conservatively (strong) at first; relax once you’re confident.

## TODO List
- [x] Fix document formatting (headings, code fences, section structure)
- [x] Add AssetRef type and JSON serialization
- [x] Add dependency model and JSON serialization
- [x] Add target profile model
- [x] Add AssetRegistry record model
- [x] Implement AssetRegistry load/save
- [x] Add DDC interface types
- [x] Implement LocalDdc storage and hashing
- [x] Add DDC key builder and fingerprint helpers
- [x] Add DDC utility hashing helpers
- [x] Define importer context/result types
- [x] Define importer interface
- [x] Add importer registry
- [x] Implement TextureImporter
- [x] Implement MeshImporter
- [x] Implement MaterialImporter
- [x] Move glTF mesh parsing to GltfImporter
- [x] Update Asset class to store importer chain and refs
- [x] Update MaterialAsset texture refs to AssetRef
- [x] Update StaticMeshAsset material slots to AssetRef
- [x] Update AssetManager to use importers and LocalDdc
- [x] Wire AssetRegistry registration on asset load/import
- [x] Update tests for AssetRef serialization
- [x] Update tests for importer-based cooking
