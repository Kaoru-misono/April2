#include "material-texture-manager.hpp"

namespace april::graphics
{
    MaterialTextureManager::MaterialTextureManager()
    {
        m_descriptors.emplace_back();
    }

    auto MaterialTextureManager::registerTexture(core::ref<Texture> texture, size_t maxCount) -> DescriptorHandle
    {
        if (!texture)
        {
            return kInvalidDescriptorHandle;
        }

        if (auto const it = m_indicesByTexture.find(texture.get()); it != m_indicesByTexture.end())
        {
            return it->second;
        }

        if (m_descriptors.size() >= maxCount)
        {
            return kInvalidDescriptorHandle;
        }

        auto const handle = static_cast<DescriptorHandle>(m_descriptors.size());
        m_descriptors.push_back(texture);
        m_indicesByTexture[texture.get()] = handle;
        return handle;
    }

    auto MaterialTextureManager::replaceTexture(DescriptorHandle handle, core::ref<Texture> texture) -> bool
    {
        if (handle == kInvalidDescriptorHandle || handle >= m_descriptors.size() || !texture)
        {
            return false;
        }

        if (auto const& previous = m_descriptors[handle])
        {
            if (previous.get() != texture.get())
            {
                m_indicesByTexture.erase(previous.get());
                previous->invalidateViews();
            }
        }

        m_descriptors[handle] = texture;
        m_indicesByTexture[texture.get()] = handle;
        return true;
    }

    auto MaterialTextureManager::getTexture(DescriptorHandle handle) const -> core::ref<Texture>
    {
        if (handle == kInvalidDescriptorHandle || handle >= m_descriptors.size())
        {
            return nullptr;
        }

        return m_descriptors[handle];
    }

    auto MaterialTextureManager::size() const -> uint32_t
    {
        return static_cast<uint32_t>(m_descriptors.size());
    }

    auto MaterialTextureManager::enqueueDeferred(DeferredTextureLoader loader) -> void
    {
        if (loader)
        {
            m_deferred.push_back(std::move(loader));
        }
    }

    auto MaterialTextureManager::resolveDeferred(size_t maxCount) -> bool
    {
        if (m_deferred.empty())
        {
            return false;
        }

        auto changed = false;
        auto remaining = std::vector<DeferredTextureLoader>{};
        remaining.reserve(m_deferred.size());

        for (auto& loader : m_deferred)
        {
            if (m_descriptors.size() >= maxCount)
            {
                remaining.push_back(std::move(loader));
                continue;
            }

            if (auto texture = loader())
            {
                changed |= registerTexture(texture, maxCount) != kInvalidDescriptorHandle;
            }
            else
            {
                remaining.push_back(std::move(loader));
            }
        }

        m_deferred = std::move(remaining);
        return changed;
    }

    auto MaterialTextureManager::hasDeferred() const -> bool
    {
        return !m_deferred.empty();
    }

    auto MaterialTextureManager::forEach(std::function<void(core::ref<Texture> const&)> const& visitor) const -> void
    {
        for (auto const& texture : m_descriptors)
        {
            visitor(texture);
        }
    }
}
