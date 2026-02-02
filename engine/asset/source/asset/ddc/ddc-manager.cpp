#include "ddc-manager.hpp"
#include "../blob-header.hpp"

#include <core/log/logger.hpp>
#include <stb/stb_image.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

#include <fstream>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <array>
#include <limits>

namespace april::asset
{
    DDCManager::DDCManager(std::filesystem::path cacheRoot)
        : m_cacheRoot{std::move(cacheRoot)}
    {
        if (!std::filesystem::exists(m_cacheRoot))
        {
            std::filesystem::create_directories(m_cacheRoot);
        }
    }

    auto DDCManager::getOrCompileTexture(TextureAsset const& asset) -> std::vector<std::byte>
    {
        auto key = asset.computeDDCKey();
        auto subDir = key.substr(0, 2);
        auto cacheDir = m_cacheRoot / subDir;
        auto cacheFile = cacheDir / (key + ".bin");

        if (std::filesystem::exists(cacheFile))
        {
            AP_INFO("[DDC] Cache hit: {}", key);
            return loadFile(cacheFile);
        }

        if (!std::filesystem::exists(cacheDir))
        {
            std::filesystem::create_directories(cacheDir);
        }

        AP_INFO("[DDC] Compiling texture: {}", asset.getSourcePath());
        auto blob = compileInternal(asset);

        if (!blob.empty())
        {
            saveFile(cacheFile, blob);
        }

        return blob;
    }

    auto DDCManager::compileInternal(TextureAsset const& asset) -> std::vector<std::byte>
    {
        auto const& sourcePath = asset.getSourcePath();
        auto const& settings = asset.m_settings;

        // Load image using stb_image
        auto width = 0;
        auto height = 0;
        auto channels = 0;

        // Request 4 channels (RGBA) for consistency
        auto constexpr desiredChannels = 4;
        auto* pixels = stbi_load(sourcePath.c_str(), &width, &height, &channels, desiredChannels);

        if (!pixels)
        {
            AP_ERROR("[DDC] Failed to load image: {} - {}", sourcePath, stbi_failure_reason());
            return {};
        }

        // Calculate data size
        auto const dataSize = static_cast<uint64_t>(width) * height * desiredChannels;

        // Build texture header
        auto header = TextureHeader{};
        header.width = static_cast<uint32_t>(width);
        header.height = static_cast<uint32_t>(height);
        header.channels = desiredChannels;
        header.format = settings.sRGB ? PixelFormat::RGBA8UnormSrgb : PixelFormat::RGBA8Unorm;
        header.mipLevels = settings.generateMips ? calculateMipLevels(width, height) : 1;
        header.flags = settings.sRGB ? 1 : 0; // Flag bit 0 = sRGB
        header.dataSize = dataSize;

        // Create blob: header + pixel data
        auto blob = std::vector<std::byte>(sizeof(TextureHeader) + dataSize);

        // Copy header
        std::memcpy(blob.data(), &header, sizeof(TextureHeader));

        // Copy pixel data
        std::memcpy(blob.data() + sizeof(TextureHeader), pixels, dataSize);

        // Free stb_image memory
        stbi_image_free(pixels);

        AP_INFO("[DDC] Compiled texture: {}x{} {} channels, {} mips, {} bytes",
                header.width, header.height, header.channels, header.mipLevels, blob.size());

        return blob;
    }

    auto DDCManager::calculateMipLevels(int width, int height) -> uint32_t
    {
        auto maxDim = std::max(width, height);
        auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(maxDim))) + 1;
        return mipLevels;
    }

    auto DDCManager::loadFile(std::filesystem::path const& path) -> std::vector<std::byte>
    {
        auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
        if (!file.is_open())
        {
            return {};
        }

        auto size = file.tellg();
        auto buffer = std::vector<std::byte>(size);
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(buffer.data()), size);
        return buffer;
    }

    auto DDCManager::saveFile(std::filesystem::path const& path, std::vector<std::byte> const& data) -> void
    {
        auto file = std::ofstream{path, std::ios::binary};
        if (file.is_open())
        {
            file.write(reinterpret_cast<char const*>(data.data()), data.size());
        }
    }

    auto DDCManager::getOrCompileMesh(StaticMeshAsset const& asset) -> std::vector<std::byte>
    {
        auto key = asset.computeDDCKey();
        auto subDir = key.substr(0, 2);
        auto cacheDir = m_cacheRoot / subDir;
        auto cacheFile = cacheDir / (key + ".bin");

        if (std::filesystem::exists(cacheFile))
        {
            AP_INFO("[DDC] Cache hit (mesh): {}", key);
            return loadFile(cacheFile);
        }

        if (!std::filesystem::exists(cacheDir))
        {
            std::filesystem::create_directories(cacheDir);
        }

        AP_INFO("[DDC] Compiling mesh: {}", asset.getSourcePath());
        auto blob = compileInternalMesh(asset);

        if (!blob.empty())
        {
            saveFile(cacheFile, blob);
        }

        return blob;
    }

    auto DDCManager::compileInternalMesh(StaticMeshAsset const& asset) -> std::vector<std::byte>
    {
        auto const& sourcePath = asset.getSourcePath();
        auto const& settings = asset.m_settings;

        // Load glTF using tinygltf
        auto loader = tinygltf::TinyGLTF{};
        auto model = tinygltf::Model{};
        auto err = std::string{};
        auto warn = std::string{};

        auto success = false;
        if (sourcePath.ends_with(".gltf"))
        {
            success = loader.LoadASCIIFromFile(&model, &err, &warn, sourcePath);
        }
        else if (sourcePath.ends_with(".glb"))
        {
            success = loader.LoadBinaryFromFile(&model, &err, &warn, sourcePath);
        }
        else
        {
            AP_ERROR("[DDC] Unsupported mesh format: {}", sourcePath);
            return {};
        }

        if (!warn.empty())
        {
            AP_WARN("[DDC] glTF warning: {}", warn);
        }

        if (!success || !err.empty())
        {
            AP_ERROR("[DDC] Failed to load glTF: {} - {}", sourcePath, err);
            return {};
        }

        // For simplicity, we'll process the first mesh with all its primitives
        if (model.meshes.empty())
        {
            AP_ERROR("[DDC] No meshes found in glTF file: {}", sourcePath);
            return {};
        }

        auto const& mesh = model.meshes[0];
        auto vertices = std::vector<float>{};
        auto indices = std::vector<uint32_t>{};
        auto submeshes = std::vector<Submesh>{};

        auto boundsMin = std::array<float, 3>{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        auto boundsMax = std::array<float, 3>{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

        auto constexpr VERTEX_STRIDE_FLOATS = 12; // pos(3) + norm(3) + tan(4) + uv(2)

        // Helper lambda to get buffer pointer and stride
        auto getBufferData = [&](int accessorIdx) -> std::pair<const uint8_t*, int> {
            if (accessorIdx < 0) return {nullptr, 0};

            const auto& accessor = model.accessors[accessorIdx];
            const auto& bufferView = model.bufferViews[accessor.bufferView];
            const auto& buffer = model.buffers[bufferView.buffer];

            const uint8_t* dataStart = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

            int stride = accessor.ByteStride(bufferView);
            if (stride == 0)
            {
                // Calculate tightly packed stride if not specified
                // Assuming standard component types for simplicity (Float for vec3/vec2/vec4)
                if (accessor.type == TINYGLTF_TYPE_VEC3) stride = 12;
                else if (accessor.type == TINYGLTF_TYPE_VEC2) stride = 8;
                else if (accessor.type == TINYGLTF_TYPE_VEC4) stride = 16;
                else if (accessor.type == TINYGLTF_TYPE_SCALAR)
                {
                     // Handle scalar types for indices
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
            submesh.materialIndex = static_cast<uint32_t>(primitive.material);

            // Get accessors and strides
            auto [posData, posStride] = getBufferData(primitive.attributes.count("POSITION") ? primitive.attributes.at("POSITION") : -1);
            auto [normData, normStride] = getBufferData(primitive.attributes.count("NORMAL") ? primitive.attributes.at("NORMAL") : -1);
            auto [uvData, uvStride] = getBufferData(primitive.attributes.count("TEXCOORD_0") ? primitive.attributes.at("TEXCOORD_0") : -1);

            if (!posData)
            {
                 AP_ERROR("[DDC] Primitive missing POSITION attribute");
                 continue;
            }

            const auto& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
            size_t vertexCount = posAccessor.count;

            for (size_t i = 0; i < vertexCount; ++i)
            {
                // Position
                const float* p = reinterpret_cast<const float*>(posData + i * posStride);

                float px = p[0] * settings.scale;
                float py = p[1] * settings.scale;
                float pz = p[2] * settings.scale;

                vertices.push_back(px);
                vertices.push_back(py);
                vertices.push_back(pz);

                // Bounds update
                boundsMin[0] = std::min(boundsMin[0], px);
                boundsMin[1] = std::min(boundsMin[1], py);
                boundsMin[2] = std::min(boundsMin[2], pz);
                boundsMax[0] = std::max(boundsMax[0], px);
                boundsMax[1] = std::max(boundsMax[1], py);
                boundsMax[2] = std::max(boundsMax[2], pz);

                // Normal
                if (normData)
                {
                    const float* n = reinterpret_cast<const float*>(normData + i * normStride);
                    vertices.push_back(n[0]);
                    vertices.push_back(n[1]);
                    vertices.push_back(n[2]);
                }
                else
                {
                    vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
                }

                // Tangent (simplified - use binormal approximation)
                // Proper implementation would use mikktspace
                if (settings.generateTangents)
                {
                    vertices.push_back(1.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(1.0f); // w = bitangent sign
                }
                else
                {
                    vertices.push_back(1.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(1.0f);
                }

                // TexCoord
                if (uvData)
                {
                    const float* uv = reinterpret_cast<const float*>(uvData + i * uvStride);
                    vertices.push_back(uv[0]);
                    vertices.push_back(uv[1]);
                }
                else
                {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                }
            }

            // Indices
            if (primitive.indices >= 0)
            {
                auto [idxData, idxStride] = getBufferData(primitive.indices);
                const auto& idxAccessor = model.accessors[primitive.indices];

                for (size_t i = 0; i < idxAccessor.count; ++i)
                {
                    uint32_t index = 0;
                    const uint8_t* ptr = idxData + i * idxStride;

                    if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                    {
                        index = *reinterpret_cast<const uint16_t*>(ptr);
                    }
                    else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                    {
                        index = *reinterpret_cast<const uint32_t*>(ptr);
                    }
                    else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                    {
                        index = *reinterpret_cast<const uint8_t*>(ptr);
                    }

                    indices.push_back(baseVertexOffset + index);
                }
            }

            submesh.indexCount = static_cast<uint32_t>(indices.size()) - submesh.indexOffset;
            submeshes.push_back(submesh);

            baseVertexOffset += static_cast<uint32_t>(vertexCount);
        }

        // Build mesh header
        auto header = MeshHeader{};
        header.vertexCount = static_cast<uint32_t>(vertices.size() / VERTEX_STRIDE_FLOATS);
        header.indexCount = static_cast<uint32_t>(indices.size());
        header.vertexStride = VERTEX_STRIDE_FLOATS * sizeof(float); // 48 bytes
        header.indexFormat = 1; // uint32
        header.submeshCount = static_cast<uint32_t>(submeshes.size());
        header.flags = 0; // No optional attributes for now
        header.boundsMin[0] = boundsMin[0];
        header.boundsMin[1] = boundsMin[1];
        header.boundsMin[2] = boundsMin[2];
        header.boundsMax[0] = boundsMax[0];
        header.boundsMax[1] = boundsMax[1];
        header.boundsMax[2] = boundsMax[2];
        header.vertexDataSize = vertices.size() * sizeof(float);
        header.indexDataSize = indices.size() * sizeof(uint32_t);

        // Create blob: header + submeshes + vertex data + index data
        auto totalSize = sizeof(MeshHeader) +
                         submeshes.size() * sizeof(Submesh) +
                         header.vertexDataSize +
                         header.indexDataSize;
        auto blob = std::vector<std::byte>(totalSize);

        auto offset = size_t{0};

        // Copy header
        std::memcpy(blob.data() + offset, &header, sizeof(MeshHeader));
        offset += sizeof(MeshHeader);

        // Copy submeshes
        std::memcpy(blob.data() + offset, submeshes.data(), submeshes.size() * sizeof(Submesh));
        offset += submeshes.size() * sizeof(Submesh);

        // Copy vertex data
        std::memcpy(blob.data() + offset, vertices.data(), header.vertexDataSize);
        offset += header.vertexDataSize;

        // Copy index data
        std::memcpy(blob.data() + offset, indices.data(), header.indexDataSize);

        AP_INFO("[DDC] Compiled mesh: {} vertices, {} indices, {} submeshes, {} bytes",
                header.vertexCount, header.indexCount, header.submeshCount, blob.size());

        return blob;
    }

} // namespace april::asset
