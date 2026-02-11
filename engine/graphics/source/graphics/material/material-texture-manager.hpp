#pragma once

#include "rhi/texture.hpp"

#include <functional>
#include <unordered_map>
#include <vector>

namespace april::graphics
{
    class MaterialTextureManager
    {
    public:
        using DescriptorHandle = uint32_t;
        using DeferredTextureLoader = std::function<core::ref<Texture>()>;

        static constexpr DescriptorHandle kInvalidDescriptorHandle = 0;

        MaterialTextureManager();

        auto registerTexture(core::ref<Texture> texture, size_t maxCount) -> DescriptorHandle;
        auto replaceTexture(DescriptorHandle handle, core::ref<Texture> texture) -> bool;
        auto getTexture(DescriptorHandle handle) const -> core::ref<Texture>;
        auto size() const -> uint32_t;

        auto enqueueDeferred(DeferredTextureLoader loader) -> void;
        auto resolveDeferred(size_t maxCount) -> bool;
        auto hasDeferred() const -> bool;

        auto forEach(std::function<void(core::ref<Texture> const&)> const& visitor) const -> void;

    private:
        std::vector<core::ref<Texture>> m_descriptors;
        std::unordered_map<Texture*, DescriptorHandle> m_indicesByTexture;
        std::vector<DeferredTextureLoader> m_deferred;
    };
}
