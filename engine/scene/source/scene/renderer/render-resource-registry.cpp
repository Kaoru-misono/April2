#include "render-resource-registry.hpp"

namespace april::scene
{
    RenderResourceRegistry::RenderResourceRegistry(core::ref<graphics::Device> device, asset::AssetManager* assetManager)
        : m_device(std::move(device))
        , m_assetManager(assetManager)
    {
        m_meshes.resize(1);
    }

    auto RenderResourceRegistry::setDevice(core::ref<graphics::Device> device) -> void
    {
        m_device = std::move(device);
    }

    auto RenderResourceRegistry::setAssetManager(asset::AssetManager* assetManager) -> void
    {
        m_assetManager = assetManager;
    }

    auto RenderResourceRegistry::getOrCreateMeshId(std::string const& assetPath) -> RenderID
    {
        if (assetPath.empty())
        {
            return kInvalidRenderID;
        }

        auto it = m_meshIdsByPath.find(assetPath);
        if (it != m_meshIdsByPath.end())
        {
            return it->second;
        }

        if (!m_device || !m_assetManager)
        {
            AP_WARN("[RenderResourceRegistry] Missing device or asset manager; cannot load mesh: {}", assetPath);
            return kInvalidRenderID;
        }

        core::ref<graphics::StaticMesh> mesh{};
        if (std::filesystem::exists(assetPath))
        {
            auto meshAsset = m_assetManager->loadAsset<asset::StaticMeshAsset>(assetPath);
            if (meshAsset)
            {
                mesh = m_device->createMeshFromAsset(*m_assetManager, *meshAsset);
                if (mesh)
                {
                    AP_INFO("[RenderResourceRegistry] Loaded mesh: {} ({} submeshes)", assetPath, mesh->getSubmeshCount());
                }
                else
                {
                    AP_ERROR("[RenderResourceRegistry] Failed to create mesh from asset: {}", assetPath);
                }
            }
            else
            {
                AP_ERROR("[RenderResourceRegistry] Failed to load mesh asset: {}", assetPath);
            }
        }
        else
        {
            AP_ERROR("[RenderResourceRegistry] Mesh asset not found: {}", assetPath);
        }

        if (!mesh)
        {
            return kInvalidRenderID;
        }

        auto const id = static_cast<RenderID>(m_meshes.size());
        m_meshes.push_back(mesh);
        m_meshIdsByPath[assetPath] = id;
        return id;
    }

    auto RenderResourceRegistry::getMesh(RenderID id) const -> core::ref<graphics::StaticMesh>
    {
        if (id == kInvalidRenderID || id >= m_meshes.size())
        {
            return nullptr;
        }
        return m_meshes[id];
    }

    auto RenderResourceRegistry::getMeshBounds(RenderID id, float3& outMin, float3& outMax) const -> bool
    {
        auto mesh = getMesh(id);
        if (!mesh)
        {
            return false;
        }

        auto const& minBounds = mesh->getBoundsMin();
        auto const& maxBounds = mesh->getBoundsMax();
        outMin = float3{minBounds[0], minBounds[1], minBounds[2]};
        outMax = float3{maxBounds[0], maxBounds[1], maxBounds[2]};
        return true;
    }
}
