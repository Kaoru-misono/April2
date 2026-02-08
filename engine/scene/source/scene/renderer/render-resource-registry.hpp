#pragma once

#include "render-types.hpp"

#include <asset/asset-manager.hpp>
#include <asset/material-asset.hpp>
#include <asset/static-mesh-asset.hpp>
#include <core/foundation/object.hpp>
#include <core/log/logger.hpp>
#include <graphics/material/material-system.hpp>
#include <graphics/material/standard-material.hpp>
#include <graphics/resources/static-mesh.hpp>
#include <graphics/rhi/render-device.hpp>

#include <unordered_map>
#include <vector>
#include <string>
#include <utility>
#include <memory>

namespace april::scene
{
    class RenderResourceRegistry
    {
    public:
        RenderResourceRegistry() = default;
        RenderResourceRegistry(core::ref<graphics::Device> device, asset::AssetManager* assetManager);

        auto setDevice(core::ref<graphics::Device> device) -> void;
        auto setAssetManager(asset::AssetManager* assetManager) -> void;

        // Mesh API
        auto registerMesh(std::string const& assetPath) -> RenderID;
        auto getMesh(RenderID id) const -> core::ref<graphics::StaticMesh>;
        auto getMeshBounds(RenderID id, float3& outMin, float3& outMax) const -> bool;
        auto getMeshMaterialId(RenderID meshId, size_t slotIndex) const -> RenderID;

        // Material API
        auto getMaterialSystem() -> graphics::MaterialSystem*;
        auto getOrCreateMaterialId(std::string const& assetPath) -> RenderID;
        auto getMaterial(RenderID id) const -> core::ref<graphics::IMaterial>;

    private:
        auto loadMaterialTextures(
            core::ref<graphics::StandardMaterial> material,
            asset::MaterialTextures const& textures
        ) -> void;

        core::ref<graphics::Device> m_device{};
        asset::AssetManager* m_assetManager{};

        // Mesh storage
        std::vector<core::ref<graphics::StaticMesh>> m_meshes{};
        std::vector<std::vector<RenderID>> m_meshMaterialIds{};
        std::unordered_map<core::UUID, RenderID> m_meshIdsByGuid{};

        // Material storage
        core::ref<graphics::MaterialSystem> m_materialSystem{};
        std::vector<core::ref<graphics::IMaterial>> m_materials{};
        std::unordered_map<core::UUID, RenderID> m_materialIdsByGuid{};

        auto registerMaterialAsset(std::shared_ptr<asset::MaterialAsset> const& materialAsset) -> RenderID;
    };
}
