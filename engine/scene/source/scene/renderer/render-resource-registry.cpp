#include "render-resource-registry.hpp"

#include <asset/texture-asset.hpp>
#include <filesystem>

namespace april::scene
{
    RenderResourceRegistry::RenderResourceRegistry(core::ref<graphics::Device> device, asset::AssetManager* assetManager)
        : m_device(std::move(device))
        , m_assetManager(assetManager)
    {
        // Reserve slot 0 for invalid
        m_meshes.resize(1);
        m_meshMaterialIds.resize(1);
        m_materials.resize(1);
        m_materialBufferIndices.resize(1, 0);

        // Create material system
        if (m_device)
        {
            m_materialSystem = core::make_ref<graphics::MaterialSystem>(m_device);

            // Reserve GPU material slot 0 as a deterministic fallback.
            auto defaultMaterial = core::make_ref<graphics::StandardMaterial>();
            m_defaultMaterialBufferIndex = m_materialSystem->addMaterial(defaultMaterial);
        }
    }

    auto RenderResourceRegistry::setDevice(core::ref<graphics::Device> device) -> void
    {
        m_device = std::move(device);
    }

    auto RenderResourceRegistry::setAssetManager(asset::AssetManager* assetManager) -> void
    {
        m_assetManager = assetManager;
    }

    auto RenderResourceRegistry::registerMesh(std::string const& assetPath) -> RenderID
    {
        if (assetPath.empty())
        {
            return kInvalidRenderID;
        }

        if (!m_device || !m_assetManager)
        {
            AP_WARN("[RenderResourceRegistry] Missing device or asset manager; cannot load mesh: {}", assetPath);
            return kInvalidRenderID;
        }

        core::ref<graphics::StaticMesh> mesh{};
        std::shared_ptr<asset::StaticMeshAsset> meshAsset{};
        if (std::filesystem::exists(assetPath))
        {
            meshAsset = m_assetManager->loadAsset<asset::StaticMeshAsset>(assetPath);
        }
        else
        {
            AP_ERROR("[RenderResourceRegistry] Mesh asset not found: {}", assetPath);
        }

        if (!meshAsset)
        {
            AP_ERROR("[RenderResourceRegistry] Failed to load mesh asset: {}", assetPath);
            return kInvalidRenderID;
        }

        auto const meshGuid = meshAsset->getHandle();
        auto existing = m_meshIdsByGuid.find(meshGuid);
        if (existing != m_meshIdsByGuid.end())
        {
            auto const id = existing->second;
            if (id < m_meshMaterialIds.size() && m_meshMaterialIds[id].empty())
            {
                auto const slotCount = meshAsset->m_materialSlots.size();
                m_meshMaterialIds[id].resize(slotCount, kInvalidRenderID);
                for (size_t i = 0; i < slotCount; ++i)
                {
                    auto const& slot = meshAsset->m_materialSlots[i];
                    if (slot.materialRef.guid.getNative().is_nil())
                    {
                        continue;
                    }

                    auto materialAsset = m_assetManager->getAsset<asset::MaterialAsset>(slot.materialRef.guid);
                    auto materialId = registerMaterialAsset(materialAsset);
                    m_meshMaterialIds[id][i] = materialId;
                }
            }

            return id;
        }

        mesh = m_device->createMeshFromAsset(*m_assetManager, *meshAsset);
        if (mesh)
        {
            AP_INFO("[RenderResourceRegistry] Loaded mesh: {} ({} submeshes)", assetPath, mesh->getSubmeshCount());
        }
        else
        {
            AP_ERROR("[RenderResourceRegistry] Failed to create mesh from asset: {}", assetPath);
        }

        if (!mesh)
        {
            return kInvalidRenderID;
        }

        auto const id = static_cast<RenderID>(m_meshes.size());
        m_meshes.push_back(mesh);
        m_meshMaterialIds.emplace_back();
        m_meshIdsByGuid[meshGuid] = id;

        if (meshAsset)
        {
            auto const slotCount = meshAsset->m_materialSlots.size();
            m_meshMaterialIds[id].resize(slotCount, kInvalidRenderID);

            for (size_t i = 0; i < slotCount; ++i)
            {
                auto const& slot = meshAsset->m_materialSlots[i];
                if (slot.materialRef.guid.getNative().is_nil())
                {
                    continue;
                }

                auto materialAsset = m_assetManager->getAsset<asset::MaterialAsset>(slot.materialRef.guid);
                auto materialId = registerMaterialAsset(materialAsset);
                m_meshMaterialIds[id][i] = materialId;
            }
        }

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

    auto RenderResourceRegistry::getMeshMaterialId(RenderID meshId, size_t slotIndex) const -> RenderID
    {
        if (meshId == kInvalidRenderID || meshId >= m_meshMaterialIds.size())
        {
            return kInvalidRenderID;
        }

        auto const& slots = m_meshMaterialIds[meshId];
        if (slotIndex >= slots.size())
        {
            return kInvalidRenderID;
        }

        return slots[slotIndex];
    }

    auto RenderResourceRegistry::getMaterialSystem() -> graphics::MaterialSystem*
    {
        return m_materialSystem.get();
    }

    auto RenderResourceRegistry::getOrCreateMaterialId(std::string const& assetPath) -> RenderID
    {
        if (assetPath.empty())
        {
            return kInvalidRenderID;
        }

        if (!m_device || !m_assetManager || !m_materialSystem)
        {
            AP_WARN("[RenderResourceRegistry] Missing device, asset manager, or material system; cannot load material: {}", assetPath);
            return kInvalidRenderID;
        }

        std::shared_ptr<asset::MaterialAsset> materialAsset{};
        if (std::filesystem::exists(assetPath))
        {
            materialAsset = m_assetManager->loadAsset<asset::MaterialAsset>(assetPath);
        }
        else
        {
            AP_ERROR("[RenderResourceRegistry] Material asset not found: {}", assetPath);
        }

        if (!materialAsset)
        {
            AP_ERROR("[RenderResourceRegistry] Failed to load material asset: {}", assetPath);
            return kInvalidRenderID;
        }

        return registerMaterialAsset(materialAsset);
    }

    auto RenderResourceRegistry::registerMaterialAsset(
        std::shared_ptr<asset::MaterialAsset> const& materialAsset
    ) -> RenderID
    {
        if (!materialAsset)
        {
            return kInvalidRenderID;
        }

        if (!m_device || !m_assetManager || !m_materialSystem)
        {
            AP_WARN("[RenderResourceRegistry] Missing device, asset manager, or material system; cannot load material.");
            return kInvalidRenderID;
        }

        auto const assetGuid = materialAsset->getHandle();
        if (!assetGuid.getNative().is_nil())
        {
            auto it = m_materialIdsByGuid.find(assetGuid);
            if (it != m_materialIdsByGuid.end())
            {
                return it->second;
            }
        }

        auto const& assetPath = materialAsset->getAssetPath();

        // Use explicit materialType from asset metadata (no path-based heuristics).
        auto const& materialTypeName = materialAsset->materialType;

        core::ref<graphics::IMaterial> material{};
        if (materialTypeName == "Unlit")
        {
            auto unlit = core::make_ref<graphics::UnlitMaterial>();
            unlit->color = materialAsset->parameters.baseColorFactor;
            unlit->emissive = materialAsset->parameters.emissiveFactor;
            unlit->setDoubleSided(materialAsset->parameters.doubleSided);
            material = unlit;
        }
        else
        {
            // Default to Standard material for "Standard" or unknown types.
            if (materialTypeName != "Standard" && !materialTypeName.empty())
            {
                AP_WARN("[RenderResourceRegistry] Unknown material type '{}' in asset: {}; defaulting to Standard",
                    materialTypeName, assetPath);
            }
            material = graphics::StandardMaterial::createFromAsset(m_device, *materialAsset);
        }

        if (!material)
        {
            AP_ERROR("[RenderResourceRegistry] Failed to create material from asset: {}", assetPath);
            return kInvalidRenderID;
        }

        if (auto standard = core::dynamic_ref_cast<graphics::StandardMaterial>(material))
        {
            loadMaterialTextures(standard, materialAsset->textures);
        }
        auto const materialBufferIndex = m_materialSystem->addMaterial(material);

        auto const id = static_cast<RenderID>(m_materials.size());
        m_materials.push_back(material);
        m_materialBufferIndices.push_back(materialBufferIndex);
        if (!assetGuid.getNative().is_nil())
        {
            m_materialIdsByGuid[assetGuid] = id;
        }
        return id;
    }

    auto RenderResourceRegistry::getMaterial(RenderID id) const -> core::ref<graphics::IMaterial>
    {
        if (id == kInvalidRenderID || id >= m_materials.size())
        {
            return nullptr;
        }
        return m_materials[id];
    }

    auto RenderResourceRegistry::getMaterialBufferIndex(RenderID id) const -> uint32_t
    {
        if (id == kInvalidRenderID || id >= m_materialBufferIndices.size())
        {
            return m_defaultMaterialBufferIndex;
        }

        return m_materialBufferIndices[id];
    }

    auto RenderResourceRegistry::resolveGpuMaterialIndex(
        RenderID meshId,
        uint32_t slotIndex,
        RenderID overrideMaterialId
    ) const -> uint32_t
    {
        auto materialId = overrideMaterialId;
        if (materialId == kInvalidRenderID)
        {
            materialId = getMeshMaterialId(meshId, slotIndex);
        }

        return getMaterialBufferIndex(materialId);
    }

    auto RenderResourceRegistry::getMaterialTypeId(RenderID id) const -> uint32_t
    {
        auto const bufferIndex = getMaterialBufferIndex(id);
        if (!m_materialSystem)
        {
            return 0;
        }

        return m_materialSystem->getMaterialTypeId(bufferIndex);
    }

    auto RenderResourceRegistry::getMaterialTypeName(RenderID id) const -> std::string
    {
        auto const typeId = getMaterialTypeId(id);
        if (!m_materialSystem)
        {
            return "Unknown";
        }

        return m_materialSystem->getMaterialTypeRegistry().resolveTypeName(typeId);
    }

    auto RenderResourceRegistry::loadMaterialTextures(
        core::ref<graphics::StandardMaterial> material,
        asset::MaterialTextures const& textures
    ) -> void
    {
        if (!material || !m_device || !m_assetManager)
        {
            return;
        }

        auto loadTexture = [&](std::optional<asset::TextureReference> const& ref) -> core::ref<graphics::Texture> {
            if (!ref.has_value() || ref->asset.guid.getNative().is_nil())
            {
                return nullptr;
            }

            auto const& guid = ref->asset.guid;
            auto textureAsset = m_assetManager->getAsset<asset::TextureAsset>(guid);
            if (!textureAsset)
            {
                AP_WARN("[RenderResourceRegistry] Failed to load texture asset by GUID: {}", guid.toString());
                return nullptr;
            }

            auto texture = m_device->createTextureFromAsset(*m_assetManager, *textureAsset);
            if (!texture)
            {
                AP_WARN("[RenderResourceRegistry] Failed to create texture from asset: {}", guid.toString());
            }
            return texture;
        };

        material->baseColorTexture = loadTexture(textures.baseColorTexture);
        material->metallicRoughnessTexture = loadTexture(textures.metallicRoughnessTexture);
        material->normalTexture = loadTexture(textures.normalTexture);
        material->occlusionTexture = loadTexture(textures.occlusionTexture);
        material->emissiveTexture = loadTexture(textures.emissiveTexture);
    }
}
