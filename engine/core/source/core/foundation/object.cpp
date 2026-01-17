#include "object.hpp"
#include "log/logger.hpp"

namespace april::core
{

#if APRIL_ENABLE_OBJECT_TRACKING
    static std::mutex s_trackedObjectsMutex;
    static std::set<Object const*> s_trackedObjects;
#endif

    auto Object::incRef() const -> void
    {
        uint32_t refCount = m_refCount.fetch_add(1);
        if (refCount == 0)
        {
#if APRIL_ENABLE_OBJECT_TRACKING
            std::lock_guard<std::mutex> lock(s_trackedObjectsMutex);
            s_trackedObjects.insert(this);
#endif
        }
    }

    auto Object::decRef(bool dealloc) const noexcept -> void
    {
        uint32_t refCount = m_refCount.fetch_sub(1);
        if (refCount <= 0)
        {
            AP_CRITICAL("Internal error: Object reference count < 0!");
            std::terminate();
        }
        else if (refCount == 1)
        {
#if APRIL_ENABLE_OBJECT_TRACKING
            {
                std::lock_guard<std::mutex> lock(s_trackedObjectsMutex);
                s_trackedObjects.erase(this);
            }
#endif
            if (dealloc)
                delete this;
        }
    }

#if APRIL_ENABLE_OBJECT_TRACKING
    auto Object::dumpAliveObjects() -> void
    {
        std::lock_guard<std::mutex> lock(s_trackedObjectsMutex);
        AP_INFO("Alive objects:");
        for (Object const* object : s_trackedObjects)
        {
            object->dumpRefs();
        }
    }

    auto Object::dumpRefs() const -> void
    {
        AP_INFO("Object (class={} address={}) has {} reference(s)", getClassName(), (void*)this, refCount());
#if APRIL_ENABLE_REF_TRACKING
        std::lock_guard<std::mutex> lock(m_refTrackerMutex);
        for (auto const& it : m_refTrackers)
        {
            AP_INFO("ref={} count={}\n{}\n", it.first, it.second.count, it.second.origin);
        }
#endif
    }
#endif // APRIL_ENABLE_OBJECT_TRACKING

#if APRIL_ENABLE_REF_TRACKING
    // Falcor's getStackTrace() is used in Object.cpp.
    // Since I don't have getStackTrace implementation yet, and the macros are 0 by default,
    // I will use a simple placeholder if enabled.
    static auto getStackTrace() -> std::string { return "Stack trace not implemented"; }

    auto Object::incRef(uint64_t refId) const -> void
    {
        if (m_enableRefTracking)
        {
            std::lock_guard<std::mutex> lock(m_refTrackerMutex);
            auto it = m_refTrackers.find(refId);
            if (it != m_refTrackers.end())
            {
                it->second.count++;
            }
            else
            {
                m_refTrackers.emplace(refId, getStackTrace());
            }
        }

        incRef();
    }

    auto Object::decRef(uint64_t refId, bool dealloc) const noexcept -> void
    {
        if (m_enableRefTracking)
        {
            std::lock_guard<std::mutex> lock(m_refTrackerMutex);
            auto it = m_refTrackers.find(refId);
            // Using check/assert here equivalent to FALCOR_ASSERT
            if (it == m_refTrackers.end())
            {
                 AP_ERROR("Object::decRef: refId not found in ref trackers");
            }
            else
            {
                if (--it->second.count == 0)
                {
                    m_refTrackers.erase(it);
                }
            }
        }

        decRef(dealloc);
    }

    auto Object::setEnableRefTracking(bool enable) -> void
    {
        m_enableRefTracking = enable;
    }
#endif // APRIL_ENABLE_REF_TRACKING

} // namespace april::core
