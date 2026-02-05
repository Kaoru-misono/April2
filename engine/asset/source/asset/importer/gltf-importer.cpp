#include "gltf-importer.hpp"

#include <core/log/logger.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

#include <meshoptimizer.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <format>
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

            auto extension = sourcePath.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            auto success = false;
            if (extension == ".gltf")
            {
                success = loader.LoadASCIIFromFile(&outModel, &err, &warn, sourcePath.string());
            }
            else if (extension == ".glb")
            {
                success = loader.LoadBinaryFromFile(&outModel, &err, &warn, sourcePath.string());
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
            if (!std::filesystem::exists(texturePath))
            {
                AP_WARN("[GltfImporter] Texture file not found: {}", texturePath.string());
                return std::nullopt;
            }

            return GltfTextureSource{texturePath, textureInfo.texCoord};
        }
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

                if (settings.generateTangents)
                {
                    vertices.push_back(1.0f);
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
} // namespace april::asset
