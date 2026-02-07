#pragma once

#include "render-types.hpp"

#include <asset/asset-manager.hpp>
#include <asset/static-mesh-asset.hpp>
#include <core/foundation/object.hpp>
#include <core/log/logger.hpp>
#include <graphics/resources/static-mesh.hpp>
#include <graphics/rhi/render-device.hpp>

#include <unordered_map>
#include <vector>
#include <string>

namespace april::scene
{
    class RenderResourceRegistry
    {
    public:
        RenderResourceRegistry() = default;
        RenderResourceRegistry(core::ref<graphics::Device> device, asset::AssetManager* assetManager);

        auto setDevice(core::ref<graphics::Device> device) -> void;
        auto setAssetManager(asset::AssetManager* assetManager) -> void;
        auto getOrCreateMeshId(std::string const& assetPath) -> RenderID;
        auto getMesh(RenderID id) const -> core::ref<graphics::StaticMesh>;
        auto getMeshBounds(RenderID id, float3& outMin, float3& outMax) const -> bool;

    private:
        core::ref<graphics::Device> m_device{};
        asset::AssetManager* m_assetManager{};
        std::vector<core::ref<graphics::StaticMesh>> m_meshes{};
        std::unordered_map<std::string, RenderID> m_meshIdsByPath{};
    };
}
