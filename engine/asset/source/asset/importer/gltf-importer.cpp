#include "gltf-importer.hpp"

#include "../ddc/ddc-key.hpp"
#include "../ddc/ddc-utils.hpp"

#include <core/file/vfs.hpp>
#include <core/log/logger.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <meshoptimizer.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <cmath>
#include <format>
#include <span>
#include <limits>

namespace april::asset
{
    namespace
    {
        auto loadModel(std::filesystem::path const& sourcePath, tinygltf::Model& outModel) -> bool
        {
            auto loader = tinygltf::TinyGLTF{};
            auto err = std::string{};
            auto warn = std::string{};

            auto callbacks = tinygltf::FsCallbacks{};
            callbacks.FileExists = [](std::string const& path, void*) -> bool
            {
                return VFS::existsFile(path);
            };
            callbacks.ReadWholeFile = [](std::vector<unsigned char>* out,
                                         std::string* outErr,
                                         std::string const& path,
                                         void*) -> bool
            {
                auto data = VFS::readBinaryFile(path);
                if (data.empty())
                {
                    if (outErr)
                    {
                        *outErr = std::format("Failed to read file: {}", path);
                    }
                    return false;
                }

                out->assign(reinterpret_cast<unsigned char const*>(data.data()),
                    reinterpret_cast<unsigned char const*>(data.data() + data.size()));
                return true;
            };
            callbacks.WriteWholeFile = [](std::string* outErr,
                                          std::string const& path,
                                          std::vector<unsigned char> const& data,
                                          void*) -> bool
            {
                auto span = std::span<std::byte const>{
                    reinterpret_cast<std::byte const*>(data.data()),
                    data.size()};
                if (!VFS::writeBinaryFile(path, span))
                {
                    if (outErr)
                    {
                        *outErr = std::format("Failed to write file: {}", path);
                    }
                    return false;
                }

                return true;
            };
            callbacks.ExpandFilePath = [](std::string const& path, void*) -> std::string
            {
                return path;
            };
            callbacks.GetFileSizeInBytes = [](size_t* fileSize,
                                              std::string* outErr,
                                              std::string const& path,
                                              void*) -> bool
            {
                auto file = VFS::open(path);
                if (!file)
                {
                    if (outErr)
                    {
                        *outErr = std::format("Failed to open file: {}", path);
                    }
                    return false;
                }

                *fileSize = file->getSize();
                return true;
            };
            callbacks.user_data = nullptr;
            loader.SetFsCallbacks(callbacks);

            auto extension = sourcePath.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            auto success = false;
            if (extension == ".gltf")
            {
                auto payload = VFS::readTextFile(sourcePath.string());
                if (payload.empty())
                {
                    AP_ERROR("[GltfImporter] Failed to open glTF file: {}", sourcePath.string());
                    return false;
                }

                auto baseDir = VFS::resolvePath(sourcePath.parent_path().string());
                success = loader.LoadASCIIFromString(
                    &outModel,
                    &err,
                    &warn,
                    payload.c_str(),
                    static_cast<unsigned int>(payload.size()),
                    std::filesystem::absolute(baseDir).string());
            }
            else if (extension == ".glb")
            {
                auto payload = VFS::readBinaryFile(sourcePath.string());
                if (payload.empty())
                {
                    AP_ERROR("[GltfImporter] Failed to open glTF file: {}", sourcePath.string());
                    return false;
                }

                auto baseDir = VFS::resolvePath(sourcePath.parent_path().string());
                success = loader.LoadBinaryFromMemory(
                    &outModel,
                    &err,
                    &warn,
                    reinterpret_cast<unsigned char const*>(payload.data()),
                    static_cast<unsigned int>(payload.size()),
                    std::filesystem::absolute(baseDir).string());
            }
            else
            {
                AP_ERROR("[GltfImporter] Unsupported mesh format: {}", sourcePath.string());
                return false;
            }

            if (!warn.empty())
            {
                AP_WARN("[GltfImporter] glTF warning: {}", warn);
            }

            if (!success || !err.empty())
            {
                AP_ERROR("[GltfImporter] Failed to load glTF: {} - {}", sourcePath.string(), err);
                return false;
            }

            return true;
        }

        template <typename T>
        auto resolveTextureSource(
            tinygltf::Model const& model,
            T const& textureInfo,
            std::filesystem::path const& baseDir
        ) -> std::optional<GltfTextureSource>
        {
            if (textureInfo.index < 0)
            {
                return std::nullopt;
            }

            if (textureInfo.index >= static_cast<int>(model.textures.size()))
            {
                AP_WARN("[GltfImporter] Invalid texture index: {}", textureInfo.index);
                return std::nullopt;
            }

            auto const& texture = model.textures[textureInfo.index];
            if (texture.source < 0 || texture.source >= static_cast<int>(model.images.size()))
            {
                AP_WARN("[GltfImporter] Invalid texture source index: {}", texture.source);
                return std::nullopt;
            }

            auto const& image = model.images[texture.source];
            if (image.uri.empty())
            {
                AP_WARN("[GltfImporter] Embedded texture not supported for import");
                return std::nullopt;
            }

            if (image.uri.starts_with("data:"))
            {
                AP_WARN("[GltfImporter] Data URI texture not supported for import");
                return std::nullopt;
            }

            auto texturePath = baseDir / image.uri;
            if (!VFS::existsFile(texturePath.string()))
            {
                AP_WARN("[GltfImporter] Texture file not found: {}", texturePath.string());
                return std::nullopt;
            }

            return GltfTextureSource{texturePath, textureInfo.texCoord};
        }

        auto sanitizeAssetName(std::string name) -> std::string
        {
            if (name.empty())
            {
                return "material";
            }

            for (auto& ch : name)
            {
                if (ch == '\\' || ch == '/' || ch == ':' || ch == '*' || ch == '?' ||
                    ch == '"' || ch == '<' || ch == '>' || ch == '|')
                {
                    ch = '_';
                }
            }

            return name;
        }

        auto normalizeVec3(std::array<float, 3> const& value) -> std::array<float, 3>
        {
            auto const lengthSq = value[0] * value[0] + value[1] * value[1] + value[2] * value[2];
            if (lengthSq <= 0.0f)
            {
                return {0.0f, 0.0f, 0.0f};
            }

            auto const invLen = 1.0f / std::sqrt(lengthSq);
            return {value[0] * invLen, value[1] * invLen, value[2] * invLen};
        }

        auto crossVec3(std::array<float, 3> const& a, std::array<float, 3> const& b) -> std::array<float, 3>
        {
            return {
                a[1] * b[2] - a[2] * b[1],
                a[2] * b[0] - a[0] * b[2],
                a[0] * b[1] - a[1] * b[0]
            };
        }

        auto dotVec3(std::array<float, 3> const& a, std::array<float, 3> const& b) -> float
        {
            return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
        }

        auto generateTangents(std::vector<float>& vertices, std::vector<uint32_t> const& indices, size_t vertexStrideFloats) -> bool
        {
            if (vertexStrideFloats < 12 || indices.size() < 3)
            {
                return false;
            }

            auto const vertexCount = vertices.size() / vertexStrideFloats;
            if (vertexCount == 0)
            {
                return false;
            }

            auto tan1 = std::vector<std::array<float, 3>>(vertexCount, {0.0f, 0.0f, 0.0f});
            auto tan2 = std::vector<std::array<float, 3>>(vertexCount, {0.0f, 0.0f, 0.0f});

            auto fetchVec3 = [&](size_t vertexIndex, size_t offset) -> std::array<float, 3>
            {
                auto const base = vertexIndex * vertexStrideFloats + offset;
                return {vertices[base + 0], vertices[base + 1], vertices[base + 2]};
            };

            auto fetchVec2 = [&](size_t vertexIndex, size_t offset) -> std::array<float, 2>
            {
                auto const base = vertexIndex * vertexStrideFloats + offset;
                return {vertices[base + 0], vertices[base + 1]};
            };

            for (size_t i = 0; i + 2 < indices.size(); i += 3)
            {
                auto const i0 = static_cast<size_t>(indices[i + 0]);
                auto const i1 = static_cast<size_t>(indices[i + 1]);
                auto const i2 = static_cast<size_t>(indices[i + 2]);
                if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount)
                {
                    continue;
                }

                auto const p0 = fetchVec3(i0, 0);
                auto const p1 = fetchVec3(i1, 0);
                auto const p2 = fetchVec3(i2, 0);

                auto const uv0 = fetchVec2(i0, 10);
                auto const uv1 = fetchVec2(i1, 10);
                auto const uv2 = fetchVec2(i2, 10);

                auto const x1 = p1[0] - p0[0];
                auto const y1 = p1[1] - p0[1];
                auto const z1 = p1[2] - p0[2];
                auto const x2 = p2[0] - p0[0];
                auto const y2 = p2[1] - p0[1];
                auto const z2 = p2[2] - p0[2];

                auto const s1 = uv1[0] - uv0[0];
                auto const t1 = uv1[1] - uv0[1];
                auto const s2 = uv2[0] - uv0[0];
                auto const t2 = uv2[1] - uv0[1];

                auto const denom = (s1 * t2 - s2 * t1);
                if (std::abs(denom) < 1e-8f)
                {
                    continue;
                }

                auto const r = 1.0f / denom;
                auto const sdir = std::array<float, 3>{
                    (x1 * t2 - x2 * t1) * r,
                    (y1 * t2 - y2 * t1) * r,
                    (z1 * t2 - z2 * t1) * r
                };
                auto const tdir = std::array<float, 3>{
                    (x2 * s1 - x1 * s2) * r,
                    (y2 * s1 - y1 * s2) * r,
                    (z2 * s1 - z1 * s2) * r
                };

                for (auto const idx : {i0, i1, i2})
                {
                    tan1[idx][0] += sdir[0];
                    tan1[idx][1] += sdir[1];
                    tan1[idx][2] += sdir[2];
                    tan2[idx][0] += tdir[0];
                    tan2[idx][1] += tdir[1];
                    tan2[idx][2] += tdir[2];
                }
            }

            for (size_t i = 0; i < vertexCount; ++i)
            {
                auto const normal = normalizeVec3(fetchVec3(i, 3));
                auto tangent = tan1[i];

                auto const nDotT = dotVec3(normal, tangent);
                tangent = {
                    tangent[0] - normal[0] * nDotT,
                    tangent[1] - normal[1] * nDotT,
                    tangent[2] - normal[2] * nDotT
                };
                tangent = normalizeVec3(tangent);

                auto w = 1.0f;
                auto const bitangent = tan2[i];
                if (dotVec3(crossVec3(normal, tangent), bitangent) < 0.0f)
                {
                    w = -1.0f;
                }

                if (tangent[0] == 0.0f && tangent[1] == 0.0f && tangent[2] == 0.0f)
                {
                    tangent = {1.0f, 0.0f, 0.0f};
                    w = 1.0f;
                }

                auto const base = i * vertexStrideFloats + 6;
                vertices[base + 0] = tangent[0];
                vertices[base + 1] = tangent[1];
                vertices[base + 2] = tangent[2];
                vertices[base + 3] = w;
            }

            return true;
        }
    }

    auto GltfImporter::supportsExtension(std::string_view extension) const -> bool
    {
        return extension == ".gltf" || extension == ".glb";
    }

    auto GltfImporter::import(ImportSourceContext const& context) -> ImportSourceResult
    {
        auto result = ImportSourceResult{};

        if (context.sourcePath.empty())
        {
            result.errors.push_back("Missing source path for glTF import");
            return result;
        }

        auto sourcePath = context.sourcePath;

        auto meshAsset = std::make_shared<StaticMeshAsset>();
        meshAsset->setSourcePath(sourcePath.string());
        meshAsset->setAssetPath(sourcePath.string() + ".asset");

        if (!context.importMaterials)
        {
            result.primaryAsset = meshAsset;
            result.assets.push_back(meshAsset);
            return result;
        }

        auto materialsData = importMaterials(sourcePath);
        if (!materialsData)
        {
            result.errors.push_back("Failed to load GLTF materials");
            return result;
        }
        auto materialSlots = std::vector<MaterialSlot>{};
        auto materialAssets = std::vector<std::shared_ptr<MaterialAsset>>{};
        auto textureRefs = context.importTextures ? importTextures(*materialsData, context)
                                                   : std::unordered_map<std::string, AssetRef>{};
        materialSlots = importMaterialAssets(
            *materialsData,
            textureRefs,
            sourcePath.parent_path(),
            context,
            materialAssets
        );

        meshAsset->m_materialSlots = materialSlots;

        auto meshRefs = std::vector<AssetRef>{};
        meshRefs.reserve(materialSlots.size());
        for (auto const& slot : materialSlots)
        {
            meshRefs.push_back(slot.materialRef);
        }
        meshAsset->setReferences(std::move(meshRefs));

        result.primaryAsset = meshAsset;
        result.assets.push_back(meshAsset);
        for (auto const& material : materialAssets)
        {
            result.assets.push_back(material);
        }

        return result;
    }

    auto GltfImporter::cook(ImportCookContext const& context) -> ImportCookResult
    {
        context.deps.deps.clear();

        auto const& asset = static_cast<StaticMeshAsset const&>(context.asset);
        auto sourcePath = std::filesystem::path{
            context.sourcePath.empty() ? asset.getSourcePath() : context.sourcePath
        };

        auto result = ImportCookResult{};

        for (auto const& ref : asset.getReferences())
        {
            context.deps.addStrong(ref);
        }

        auto settingsJson = nlohmann::json{};
        settingsJson["settings"] = asset.m_settings;
        auto settingsHash = hashJson(settingsJson);

        auto sourceHash = hashFileContents(context.sourcePath.empty() ? asset.getSourcePath() : context.sourcePath);
        auto depsHash = hashDependencies(context.deps.deps);

        constexpr auto kMeshToolchainTag = "tinygltf@unknown|meshopt@unknown|meshblob@1";
        auto key = buildDdcKey(FingerprintInput{
            "MS",
            asset.getHandle().toString(),
            std::string{id()},
            version(),
            kMeshToolchainTag,
            sourceHash,
            settingsHash,
            depsHash,
            context.target
        });

        if (!context.forceReimport && context.ddc.exists(key))
        {
            result.producedKeys.push_back(key);
            return result;
        }

        auto meshData = importMesh(sourcePath, asset.m_settings);
        if (!meshData)
        {
            result.errors.push_back("Mesh import failed");
            return result;
        }

        key = compileMesh(*meshData, context);
        if (key.empty())
        {
            result.errors.push_back("Failed to compile mesh");
            return result;
        }

        result.producedKeys.push_back(key);
        return result;
    }

    auto GltfImporter::importMesh(
        std::filesystem::path const& sourcePath,
        MeshImportSettings const& settings
    ) const -> std::optional<GltfMeshData>
    {
        auto model = tinygltf::Model{};
        if (!loadModel(sourcePath, model))
        {
            return std::nullopt;
        }

        if (model.meshes.empty())
        {
            AP_ERROR("[GltfImporter] No meshes found in glTF file: {}", sourcePath.string());
            return std::nullopt;
        }

        auto const& mesh = model.meshes[0];
        auto vertices = std::vector<float>{};
        auto indices = std::vector<uint32_t>{};
        auto submeshes = std::vector<Submesh>{};

        auto boundsMin = std::array<float, 3>{
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        };
        auto boundsMax = std::array<float, 3>{
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest()
        };

        auto constexpr vertexStrideFloats = 12; // pos(3) + norm(3) + tan(4) + uv(2)
        auto canGenerateTangents = settings.generateTangents;

        auto getBufferData = [&](int accessorIdx) -> std::pair<uint8_t const*, int>
        {
            if (accessorIdx < 0) return {nullptr, 0};

            auto const& accessor = model.accessors[accessorIdx];
            auto const& bufferView = model.bufferViews[accessor.bufferView];
            auto const& buffer = model.buffers[bufferView.buffer];

            uint8_t const* dataStart = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

            auto stride = accessor.ByteStride(bufferView);
            if (stride == 0)
            {
                if (accessor.type == TINYGLTF_TYPE_VEC3) stride = 12;
                else if (accessor.type == TINYGLTF_TYPE_VEC2) stride = 8;
                else if (accessor.type == TINYGLTF_TYPE_VEC4) stride = 16;
                else if (accessor.type == TINYGLTF_TYPE_SCALAR)
                {
                    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) stride = 2;
                    else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) stride = 4;
                    else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) stride = 1;
                }
            }
            return {dataStart, stride};
        };

        auto baseVertexOffset = 0u;

        for (auto const& primitive : mesh.primitives)
        {
            auto submesh = Submesh{};
            submesh.indexOffset = static_cast<uint32_t>(indices.size());
            submesh.materialIndex = primitive.material < 0
                ? 0u
                : static_cast<uint32_t>(primitive.material);

            auto [posData, posStride] = getBufferData(
                primitive.attributes.count("POSITION") ? primitive.attributes.at("POSITION") : -1);
            auto [normData, normStride] = getBufferData(
                primitive.attributes.count("NORMAL") ? primitive.attributes.at("NORMAL") : -1);
            auto [uvData, uvStride] = getBufferData(
                primitive.attributes.count("TEXCOORD_0") ? primitive.attributes.at("TEXCOORD_0") : -1);
            auto const hasTangentInputs = normData && uvData;
            if (!hasTangentInputs)
            {
                canGenerateTangents = false;
            }

            if (!posData)
            {
                AP_ERROR("[GltfImporter] Primitive missing POSITION attribute");
                continue;
            }

            auto const& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
            auto vertexCount = posAccessor.count;

            for (size_t i = 0; i < vertexCount; ++i)
            {
                float const* p = reinterpret_cast<float const*>(posData + i * posStride);

                auto px = p[0] * settings.scale;
                auto py = p[1] * settings.scale;
                auto pz = p[2] * settings.scale;

                vertices.push_back(px);
                vertices.push_back(py);
                vertices.push_back(pz);

                boundsMin[0] = std::min(boundsMin[0], px);
                boundsMin[1] = std::min(boundsMin[1], py);
                boundsMin[2] = std::min(boundsMin[2], pz);
                boundsMax[0] = std::max(boundsMax[0], px);
                boundsMax[1] = std::max(boundsMax[1], py);
                boundsMax[2] = std::max(boundsMax[2], pz);

                if (normData)
                {
                    float const* n = reinterpret_cast<float const*>(normData + i * normStride);
                    vertices.push_back(n[0]);
                    vertices.push_back(n[1]);
                    vertices.push_back(n[2]);
                }
                else
                {
                    vertices.push_back(0.0f);
                    vertices.push_back(1.0f);
                    vertices.push_back(0.0f);
                }

                if (settings.generateTangents && hasTangentInputs)
                {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(1.0f);
                }
                else
                {
                    vertices.push_back(1.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(1.0f);
                }

                if (uvData)
                {
                    float const* uv = reinterpret_cast<float const*>(uvData + i * uvStride);
                    vertices.push_back(uv[0]);
                    vertices.push_back(uv[1]);
                }
                else
                {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                }
            }

            if (primitive.indices >= 0)
            {
                auto [idxData, idxStride] = getBufferData(primitive.indices);
                auto const& idxAccessor = model.accessors[primitive.indices];

                for (size_t i = 0; i < idxAccessor.count; ++i)
                {
                    uint32_t index = 0;
                    uint8_t const* ptr = idxData + i * idxStride;

                    if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                    {
                        index = *reinterpret_cast<uint16_t const*>(ptr);
                    }
                    else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                    {
                        index = *reinterpret_cast<uint32_t const*>(ptr);
                    }
                    else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                    {
                        index = *reinterpret_cast<uint8_t const*>(ptr);
                    }

                    indices.push_back(baseVertexOffset + index);
                }
            }

            submesh.indexCount = static_cast<uint32_t>(indices.size()) - submesh.indexOffset;
            submeshes.push_back(submesh);

            baseVertexOffset += static_cast<uint32_t>(vertexCount);
        }

        if (canGenerateTangents)
        {
            generateTangents(vertices, indices, vertexStrideFloats);
        }
        else if (settings.generateTangents)
        {
            auto const vertexCount = vertices.size() / vertexStrideFloats;
            for (size_t i = 0; i < vertexCount; ++i)
            {
                auto const base = i * vertexStrideFloats + 6;
                vertices[base + 0] = 1.0f;
                vertices[base + 1] = 0.0f;
                vertices[base + 2] = 0.0f;
                vertices[base + 3] = 1.0f;
            }
        }

        if (settings.optimize && !indices.empty() && !vertices.empty())
        {
            auto const vertexCount = static_cast<size_t>(vertices.size() / vertexStrideFloats);
            auto const indexCount = indices.size();
            auto const vertexSize = vertexStrideFloats * sizeof(float);

            AP_INFO("[GltfImporter] Applying mesh optimizations...");

            auto remap = std::vector<unsigned int>(vertexCount);
            auto const uniqueVertexCount = meshopt_generateVertexRemap(
                remap.data(),
                indices.data(),
                indexCount,
                vertices.data(),
                vertexCount,
                vertexSize
            );

            AP_INFO("[GltfImporter]   - Deduplication: {} -> {} vertices ({:.1f}% reduction)",
                    vertexCount, uniqueVertexCount,
                    100.0f * (1.0f - static_cast<float>(uniqueVertexCount) / vertexCount));

            auto remappedVertices = std::vector<float>(uniqueVertexCount * vertexStrideFloats);
            meshopt_remapVertexBuffer(
                remappedVertices.data(),
                vertices.data(),
                vertexCount,
                vertexSize,
                remap.data()
            );

            auto remappedIndices = std::vector<unsigned int>(indexCount);
            meshopt_remapIndexBuffer(
                remappedIndices.data(),
                indices.data(),
                indexCount,
                remap.data()
            );

            auto vcacheOptIndices = std::vector<unsigned int>(indexCount);
            meshopt_optimizeVertexCache(
                vcacheOptIndices.data(),
                remappedIndices.data(),
                indexCount,
                uniqueVertexCount
            );

            auto const vcacheStats = meshopt_analyzeVertexCache(
                vcacheOptIndices.data(),
                indexCount,
                uniqueVertexCount,
                32, 32, 32
            );
            AP_INFO("[GltfImporter]   - Vertex cache: ACMR={:.2f}, ATVR={:.2f}",
                    vcacheStats.acmr, vcacheStats.atvr);

            auto overdrawOptIndices = std::vector<unsigned int>(indexCount);
            meshopt_optimizeOverdraw(
                overdrawOptIndices.data(),
                vcacheOptIndices.data(),
                indexCount,
                reinterpret_cast<float const*>(remappedVertices.data()),
                uniqueVertexCount,
                vertexSize,
                1.05f
            );

            auto const overdrawStats = meshopt_analyzeOverdraw(
                overdrawOptIndices.data(),
                indexCount,
                reinterpret_cast<float const*>(remappedVertices.data()),
                uniqueVertexCount,
                vertexSize
            );
            AP_INFO("[GltfImporter]   - Overdraw: {:.2f}x (covered={}, shaded={})",
                    overdrawStats.overdraw, overdrawStats.pixels_covered, overdrawStats.pixels_shaded);

            auto finalVertices = std::vector<float>(uniqueVertexCount * vertexStrideFloats);
            auto finalIndices = std::vector<unsigned int>(indexCount);
            std::copy(overdrawOptIndices.begin(), overdrawOptIndices.end(), finalIndices.begin());

            auto const finalVertexCount = meshopt_optimizeVertexFetch(
                finalVertices.data(),
                finalIndices.data(),
                indexCount,
                remappedVertices.data(),
                uniqueVertexCount,
                vertexSize
            );

            auto const vfetchStats = meshopt_analyzeVertexFetch(
                finalIndices.data(),
                indexCount,
                finalVertexCount,
                vertexSize
            );
            AP_INFO("[GltfImporter]   - Vertex fetch: {:.2f} bytes/vertex fetched (efficiency: {:.1f}%)",
                    vfetchStats.bytes_fetched / static_cast<float>(indexCount),
                    100.0f * vertexSize / vfetchStats.bytes_fetched * indexCount / finalVertexCount);

            vertices = std::move(finalVertices);
            indices.resize(finalIndices.size());
            for (size_t i = 0; i < finalIndices.size(); ++i)
            {
                indices[i] = static_cast<uint32_t>(finalIndices[i]);
            }

            boundsMin = {
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()
            };
            boundsMax = {
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest()
            };

            for (size_t i = 0; i < finalVertexCount; ++i)
            {
                auto const px = vertices[i * vertexStrideFloats + 0];
                auto const py = vertices[i * vertexStrideFloats + 1];
                auto const pz = vertices[i * vertexStrideFloats + 2];

                boundsMin[0] = std::min(boundsMin[0], px);
                boundsMin[1] = std::min(boundsMin[1], py);
                boundsMin[2] = std::min(boundsMin[2], pz);
                boundsMax[0] = std::max(boundsMax[0], px);
                boundsMax[1] = std::max(boundsMax[1], py);
                boundsMax[2] = std::max(boundsMax[2], pz);
            }
        }

        auto result = GltfMeshData{};
        result.vertices = std::move(vertices);
        result.indices = std::move(indices);
        result.submeshes = std::move(submeshes);
        result.boundsMin = boundsMin;
        result.boundsMax = boundsMax;

        return result;
    }

    auto GltfImporter::importMaterials(
        std::filesystem::path const& sourcePath
    ) const -> std::optional<std::vector<GltfMaterialData>>
    {
        auto model = tinygltf::Model{};
        if (!loadModel(sourcePath, model))
        {
            return std::nullopt;
        }

        auto baseDir = sourcePath.parent_path();
        auto materials = std::vector<GltfMaterialData>{};
        materials.reserve(model.materials.size());

        for (size_t i = 0; i < model.materials.size(); ++i)
        {
            auto const& gltfMaterial = model.materials[i];
            auto materialName = gltfMaterial.name.empty()
                ? std::format("{}_mat{}", sourcePath.stem().string(), i)
                : gltfMaterial.name;

            auto parameters = MaterialParameters{};
            auto const& pbr = gltfMaterial.pbrMetallicRoughness;
            parameters.baseColorFactor = {
                static_cast<float>(pbr.baseColorFactor[0]),
                static_cast<float>(pbr.baseColorFactor[1]),
                static_cast<float>(pbr.baseColorFactor[2]),
                static_cast<float>(pbr.baseColorFactor[3])
            };
            parameters.metallicFactor = static_cast<float>(pbr.metallicFactor);
            parameters.roughnessFactor = static_cast<float>(pbr.roughnessFactor);
            if (gltfMaterial.emissiveFactor.size() == 3)
            {
                parameters.emissiveFactor = {
                    static_cast<float>(gltfMaterial.emissiveFactor[0]),
                    static_cast<float>(gltfMaterial.emissiveFactor[1]),
                    static_cast<float>(gltfMaterial.emissiveFactor[2])
                };
            }
            parameters.occlusionStrength = static_cast<float>(gltfMaterial.occlusionTexture.strength);
            parameters.normalScale = static_cast<float>(gltfMaterial.normalTexture.scale);
            parameters.alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);
            parameters.alphaMode = gltfMaterial.alphaMode.empty() ? "OPAQUE" : gltfMaterial.alphaMode;
            parameters.doubleSided = gltfMaterial.doubleSided;

            auto materialData = GltfMaterialData{};
            materialData.name = materialName;
            materialData.parameters = parameters;
            materialData.baseColorTexture = resolveTextureSource(model, pbr.baseColorTexture, baseDir);
            materialData.metallicRoughnessTexture = resolveTextureSource(model, pbr.metallicRoughnessTexture, baseDir);
            materialData.normalTexture = resolveTextureSource(model, gltfMaterial.normalTexture, baseDir);
            materialData.occlusionTexture = resolveTextureSource(model, gltfMaterial.occlusionTexture, baseDir);
            materialData.emissiveTexture = resolveTextureSource(model, gltfMaterial.emissiveTexture, baseDir);

            materials.push_back(std::move(materialData));
        }

        return materials;
    }

    auto GltfImporter::importTextures(
        std::vector<GltfMaterialData> const& materials,
        ImportSourceContext const& context
    ) const -> std::unordered_map<std::string, AssetRef>
    {
        auto textureRefs = std::unordered_map<std::string, AssetRef>{};

        auto collectTexture = [&](std::optional<GltfTextureSource> const& source)
        {
            if (!source)
            {
                return;
            }

            auto pathKey = source->path.string();
            if (textureRefs.contains(pathKey))
            {
                return; // Already processed
            }

            // Check for deduplication - see if texture already exists
            if (context.reuseExistingAssets && context.findAssetBySource)
            {
                auto existing = context.findAssetBySource(source->path, AssetType::Texture);
                if (existing)
                {
                    textureRefs[pathKey] = AssetRef{existing->getHandle(), 0};
                    AP_INFO("[GltfImporter] Reusing existing texture: {}", pathKey);
                    return;
                }
            }

            // Import the texture
            if (context.importSubAsset && context.importTextures)
            {
                auto textureAsset = context.importSubAsset(source->path);
                if (textureAsset && textureAsset->getType() == AssetType::Texture)
                {
                    textureRefs[pathKey] = AssetRef{textureAsset->getHandle(), 0};
                    AP_INFO("[GltfImporter] Imported texture: {}", pathKey);
                }
                else
                {
                    AP_WARN("[GltfImporter] Failed to import texture: {}", pathKey);
                }
            }
        };

        for (auto const& material : materials)
        {
            collectTexture(material.baseColorTexture);
            collectTexture(material.metallicRoughnessTexture);
            collectTexture(material.normalTexture);
            collectTexture(material.occlusionTexture);
            collectTexture(material.emissiveTexture);
        }

        return textureRefs;
    }

    auto GltfImporter::importMaterialAssets(
        std::vector<GltfMaterialData> const& materials,
        std::unordered_map<std::string, AssetRef> const& textureRefs,
        std::filesystem::path const& baseDir,
        ImportSourceContext const& context,
        std::vector<std::shared_ptr<MaterialAsset>>& outAssets
    ) const -> std::vector<MaterialSlot>
    {
        auto slots = std::vector<MaterialSlot>{};
        slots.reserve(materials.size());

        auto getTextureRef = [&](std::optional<GltfTextureSource> const& source) -> std::optional<TextureReference>
        {
            if (!source)
            {
                return std::nullopt;
            }

            auto it = textureRefs.find(source->path.string());
            if (it == textureRefs.end())
            {
                return std::nullopt;
            }

            return TextureReference{it->second, source->texCoord};
        };

        for (auto const& materialData : materials)
        {
            auto materialName = materialData.name;
            auto sanitizedName = sanitizeAssetName(materialName);
            auto materialPath = baseDir / (sanitizedName + ".material.asset");

            // Check if material already exists
            auto materialAsset = std::shared_ptr<MaterialAsset>{};
            if (context.reuseExistingAssets && context.findAssetBySource)
            {
                // For materials, we use the source path (the gltf file) + name as key
                auto existing = context.findAssetBySource(materialPath, AssetType::Material);
                if (existing)
                {
                    materialAsset = std::static_pointer_cast<MaterialAsset>(existing);
                }
            }

            if (!materialAsset)
            {
                materialAsset = std::make_shared<MaterialAsset>();
            }

            materialAsset->setSourcePath(context.sourcePath.string());
            materialAsset->setAssetPath(materialPath.string());
            materialAsset->parameters = materialData.parameters;

            auto textures = MaterialTextures{};
            textures.baseColorTexture = getTextureRef(materialData.baseColorTexture);
            textures.metallicRoughnessTexture = getTextureRef(materialData.metallicRoughnessTexture);
            textures.normalTexture = getTextureRef(materialData.normalTexture);
            textures.occlusionTexture = getTextureRef(materialData.occlusionTexture);
            textures.emissiveTexture = getTextureRef(materialData.emissiveTexture);
            materialAsset->textures = textures;

            // Build references list
            auto references = std::vector<AssetRef>{};
            if (textures.baseColorTexture) references.push_back(textures.baseColorTexture->asset);
            if (textures.metallicRoughnessTexture) references.push_back(textures.metallicRoughnessTexture->asset);
            if (textures.normalTexture) references.push_back(textures.normalTexture->asset);
            if (textures.occlusionTexture) references.push_back(textures.occlusionTexture->asset);
            if (textures.emissiveTexture) references.push_back(textures.emissiveTexture->asset);
            materialAsset->setReferences(std::move(references));

            outAssets.push_back(materialAsset);

            slots.push_back(MaterialSlot{
                materialName,
                AssetRef{materialAsset->getHandle(), 0}
            });
        }

        return slots;
    }

    auto GltfImporter::compileMesh(
        GltfMeshData const& meshData,
        ImportCookContext const& context
    ) const -> std::string
    {
        auto const& asset = static_cast<StaticMeshAsset const&>(context.asset);

        auto settingsJson = nlohmann::json{};
        settingsJson["settings"] = asset.m_settings;
        auto settingsHash = hashJson(settingsJson);

        auto sourceHash = hashFileContents(context.sourcePath.empty() ? asset.getSourcePath() : context.sourcePath);
        auto depsHash = hashDependencies(context.deps.deps);

        constexpr auto kMeshToolchainTag = "tinygltf@unknown|meshopt@unknown|meshblob@1";
        auto key = buildDdcKey(FingerprintInput{
            "MS",
            asset.getHandle().toString(),
            std::string{id()},
            version(),
            kMeshToolchainTag,
            sourceHash,
            settingsHash,
            depsHash,
            context.target
        });

        if (!context.forceReimport && context.ddc.exists(key))
        {
            return key;
        }

        auto constexpr vertexStrideFloats = 12;
        auto const& vertices = meshData.vertices;
        auto const& indices = meshData.indices;
        auto const& submeshes = meshData.submeshes;

        if (vertices.empty() || indices.empty())
        {
            AP_ERROR("[GltfImporter] Mesh data is empty");
            return {};
        }

        auto header = MeshHeader{};
        header.vertexCount = static_cast<uint32_t>(vertices.size() / vertexStrideFloats);
        header.indexCount = static_cast<uint32_t>(indices.size());
        header.vertexStride = vertexStrideFloats * sizeof(float);
        header.indexFormat = 1;
        header.submeshCount = static_cast<uint32_t>(submeshes.size());
        header.flags = 0;
        header.boundsMin[0] = meshData.boundsMin[0];
        header.boundsMin[1] = meshData.boundsMin[1];
        header.boundsMin[2] = meshData.boundsMin[2];
        header.boundsMax[0] = meshData.boundsMax[0];
        header.boundsMax[1] = meshData.boundsMax[1];
        header.boundsMax[2] = meshData.boundsMax[2];
        header.vertexDataSize = vertices.size() * sizeof(float);
        header.indexDataSize = indices.size() * sizeof(uint32_t);

        auto totalSize = sizeof(MeshHeader) +
                         submeshes.size() * sizeof(Submesh) +
                         header.vertexDataSize +
                         header.indexDataSize;

        auto blob = std::vector<std::byte>(totalSize);
        auto offset = size_t{0};

        std::memcpy(blob.data() + offset, &header, sizeof(MeshHeader));
        offset += sizeof(MeshHeader);

        std::memcpy(blob.data() + offset, submeshes.data(), submeshes.size() * sizeof(Submesh));
        offset += submeshes.size() * sizeof(Submesh);

        std::memcpy(blob.data() + offset, vertices.data(), header.vertexDataSize);
        offset += header.vertexDataSize;

        std::memcpy(blob.data() + offset, indices.data(), header.indexDataSize);

        AP_INFO("[GltfImporter] Compiled mesh: {} vertices, {} indices, {} submeshes, {} bytes",
                header.vertexCount, header.indexCount, header.submeshCount, blob.size());

        auto value = DdcValue{};
        value.bytes = std::move(blob);
        context.ddc.put(key, value);

        return key;
    }
} // namespace april::asset
