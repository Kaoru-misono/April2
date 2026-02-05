#include "mesh-importer.hpp"

#include "../blob-header.hpp"
#include "../static-mesh-asset.hpp"
#include "../ddc/ddc-key.hpp"
#include "../ddc/ddc-utils.hpp"
#include "gltf-importer.hpp"

#include <core/log/logger.hpp>

#include <cstring>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <vector>

namespace april::asset
{
    auto MeshImporter::import(ImportContext const& context) -> ImportResult
    {
        context.deps.deps.clear();

        auto const& asset = static_cast<StaticMeshAsset const&>(context.asset);
        auto sourcePath = context.sourcePath.empty() ? asset.getSourcePath() : context.sourcePath;

        auto settingsJson = nlohmann::json{};
        settingsJson["settings"] = asset.m_settings;
        auto settingsHash = hashJson(settingsJson);

        auto sourceHash = hashFileContents(sourcePath);
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

        auto result = ImportResult{};
        if (!context.forceReimport && context.ddc.exists(key))
        {
            result.producedKeys.push_back(key);
            return result;
        }

        auto importer = GltfImporter{};
        auto meshData = importer.importMesh(std::filesystem::path{sourcePath}, asset.m_settings);
        if (!meshData)
        {
            result.errors.push_back("Mesh import failed");
            return result;
        }

        auto constexpr vertexStrideFloats = 12;
        auto const& vertices = meshData->vertices;
        auto const& indices = meshData->indices;
        auto const& submeshes = meshData->submeshes;

        if (vertices.empty() || indices.empty())
        {
            result.errors.push_back("Mesh data is empty");
            return result;
        }

        auto header = MeshHeader{};
        header.vertexCount = static_cast<uint32_t>(vertices.size() / vertexStrideFloats);
        header.indexCount = static_cast<uint32_t>(indices.size());
        header.vertexStride = vertexStrideFloats * sizeof(float);
        header.indexFormat = 1;
        header.submeshCount = static_cast<uint32_t>(submeshes.size());
        header.flags = 0;
        header.boundsMin[0] = meshData->boundsMin[0];
        header.boundsMin[1] = meshData->boundsMin[1];
        header.boundsMin[2] = meshData->boundsMin[2];
        header.boundsMax[0] = meshData->boundsMax[0];
        header.boundsMax[1] = meshData->boundsMax[1];
        header.boundsMax[2] = meshData->boundsMax[2];
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

        AP_INFO("[MeshImporter] Compiled mesh: {} vertices, {} indices, {} submeshes, {} bytes",
                header.vertexCount, header.indexCount, header.submeshCount, blob.size());

        auto value = DdcValue{};
        value.bytes = std::move(blob);
        context.ddc.put(key, value);

        result.producedKeys.push_back(key);
        return result;
    }
} // namespace april::asset
