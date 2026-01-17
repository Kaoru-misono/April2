// Base class and reference counting helper for reference counted objects.
// Ported from NVIDIA Falcor
// https://github.com/NVIDIAGameWorks/Falcor

#pragma once

#include <atomic>
#include <cstdint>
#include <format>
#include <type_traits>
#include <utility>

//Enable/disable object lifetime tracking.
//When enabled, each object that derives from Object will have its
//lifetime tracked. This is useful for debugging memory leaks.

#define APRIL_ENABLE_OBJECT_TRACKING 0

// Enable/disable reference tracking.
// When enabled, all references to object that have reference tracking
// enabled using setEnableRefTracking() are tracked. Each time the reference
// count is increased, the current stack trace is stored. This helps identify
// owners of objects that are not properly releasing their references.

#define APRIL_ENABLE_REF_TRACKING 0

#if APRIL_ENABLE_REF_TRACKING
#if !APRIL_ENABLE_OBJECT_TRACKING
#error "APRIL_ENABLE_REF_TRACKING requires APRIL_ENABLE_OBJECT_TRACKING"
#endif
#endif

// API macro placeholder
#ifndef APRIL_API
#define APRIL_API
#endif

namespace april::core
{
    class APRIL_API Object
    {
    public:
        Object() = default;

        Object(Object const&) {}

        auto operator=(Object const&) -> Object& { return *this; }

        Object(Object&&) = delete;
        auto operator=(Object&&) -> Object& = delete;

        virtual ~Object() = default;

        virtual auto getClassName() const -> char const* { return "Object"; }

        auto refCount() const -> int { return m_refCount; };

        auto incRef() const -> void;

        auto decRef(bool dealloc = true) const noexcept -> void;

#if APRIL_ENABLE_OBJECT_TRACKING
        static auto dumpAliveObjects() -> void;
        auto dumpRefs() const -> void;
#endif

#if APRIL_ENABLE_REF_TRACKING
        auto incRef(uint64_t refId) const -> void;
        auto decRef(uint64_t refId, bool dealloc = true) const noexcept -> void;
        auto setEnableRefTracking(bool enable) -> void;
#endif

    private:
        mutable std::atomic<uint32_t> m_refCount{0};

#if APRIL_ENABLE_REF_TRACKING
        struct RefTracker
        {
            uint32_t count;
            std::string origin;
            RefTracker(std::string origin_) : count(1), origin(std::move(origin_)) {}
        };
        mutable std::map<uint64_t, RefTracker> m_refTrackers;
        mutable std::mutex m_refTrackerMutex;
        bool m_enableRefTracking = false;
#endif
    };

#if APRIL_ENABLE_REF_TRACKING
    static auto nextRefId() -> uint64_t
    {
        static std::atomic<uint64_t> s_nextId = 0;
        return s_nextId.fetch_add(1);
    }
#endif

#define APRIL_OBJECT(class_)                 \
public:                                       \
    auto getClassName() const -> char const* override \
    {                                         \
        return #class_;                       \
    }

    template<typename T>
    class ref
    {
    public:
        ref() {}

        ref(std::nullptr_t) {}

        template<typename T2 = T>
        explicit ref(T2* ptr) : m_ptr(ptr)
        {
            static_assert(std::is_base_of_v<Object, T2>, "Cannot create reference to object not inheriting from Object class.");
            static_assert(std::is_convertible_v<T2*, T*>, "Cannot create reference to object from unconvertible pointer type.");
            if (m_ptr)
                incRef((Object const*)(m_ptr));
        }

        ref(ref const& r) : m_ptr(r.m_ptr)
        {
            if (m_ptr)
                incRef((Object const*)(m_ptr));
        }

        template<typename T2 = T>
        ref(ref<T2> const& r) : m_ptr(r.m_ptr)
        {
            static_assert(std::is_base_of_v<Object, T>, "Cannot create reference to object not inheriting from Object class.");
            static_assert(std::is_convertible_v<T2*, T*>, "Cannot create reference to object from unconvertible reference.");
            if (m_ptr)
                incRef((Object const*)(m_ptr));
        }

        ref(ref&& r) noexcept
            : m_ptr(r.m_ptr)
#if APRIL_ENABLE_REF_TRACKING
            , m_refId(r.m_refId)
#endif
        {
            r.m_ptr = nullptr;
#if APRIL_ENABLE_REF_TRACKING
            r.m_refId = uint64_t(-1);
#endif
        }

        template<typename T2>
        ref(ref<T2>&& r) noexcept
            : m_ptr(r.m_ptr)
#if APRIL_ENABLE_REF_TRACKING
            , m_refId(r.m_refId)
#endif
        {
            static_assert(std::is_base_of_v<Object, T>, "Cannot create reference to object not inheriting from Object class.");
            static_assert(std::is_convertible_v<T2*, T*>, "Cannot create reference to object from unconvertible reference.");
            r.m_ptr = nullptr;
#if APRIL_ENABLE_REF_TRACKING
            r.m_refId = uint64_t(-1);
#endif
        }

        ~ref()
        {
            if (m_ptr)
                decRef((Object const*)(m_ptr));
        }

        auto operator=(ref const& r) noexcept -> ref&
        {
            if (r != *this)
            {
                if (r.m_ptr)
                    incRef((Object const*)(r.m_ptr));
                T* prevPtr = m_ptr;
                m_ptr = r.m_ptr;
                if (prevPtr)
                    decRef((Object const*)(prevPtr));
            }
            return *this;
        }

        template<typename T2>
        auto operator=(ref<T2> const& r) noexcept -> ref&
        {
            static_assert(std::is_convertible_v<T2*, T*>, "Cannot assign reference to object from unconvertible reference.");
            if (r != *this)
            {
                if (r.m_ptr)
                    incRef((Object const*)(r.m_ptr));
                T* prevPtr = m_ptr;
                m_ptr = r.m_ptr;
                if (prevPtr)
                    decRef((Object const*)(prevPtr));
            }
            return *this;
        }

        auto operator=(ref&& r) noexcept -> ref&
        {
            if (static_cast<void*>(&r) != this)
            {
                if (m_ptr)
                    decRef((Object const*)(m_ptr));
                m_ptr = r.m_ptr;
                r.m_ptr = nullptr;
#if APRIL_ENABLE_REF_TRACKING
                m_refId = r.m_refId;
                r.m_refId = uint64_t(-1);
#endif
            }
            return *this;
        }

        template<typename T2>
        auto operator=(ref<T2>&& r) noexcept -> ref&
        {
            static_assert(std::is_convertible_v<T2*, T*>, "Cannot move reference to object from unconvertible reference.");
            if (static_cast<void*>(&r) != this)
            {
                if (m_ptr)
                    decRef((Object const*)(m_ptr));
                m_ptr = r.m_ptr;
                r.m_ptr = nullptr;
#if APRIL_ENABLE_REF_TRACKING
                m_refId = r.m_refId;
                r.m_refId = uint64_t(-1);
#endif
            }
            return *this;
        }

        template<typename T2 = T>
        auto reset(T2* ptr = nullptr) noexcept -> void
        {
            static_assert(std::is_convertible_v<T2*, T*>, "Cannot assign reference to object from unconvertible pointer.");
            if (ptr != m_ptr)
            {
                if (ptr)
                    incRef((Object const*)(ptr));
                T* prevPtr = m_ptr;
                m_ptr = ptr;
                if (prevPtr)
                    decRef((Object const*)(prevPtr));
            }
        }

        template<typename T2 = T>
        auto operator==(ref<T2> const& r) const -> bool
        {
            static_assert(
                std::is_convertible_v<T2*, T*> || std::is_convertible_v<T*, T2*>, "Cannot compare references of non-convertible types."
            );
            return m_ptr == r.m_ptr;
        }

        template<typename T2 = T>
        auto operator!=(ref<T2> const& r) const -> bool
        {
            static_assert(
                std::is_convertible_v<T2*, T*> || std::is_convertible_v<T*, T2*>, "Cannot compare references of non-convertible types."
            );
            return m_ptr != r.m_ptr;
        }

        template<typename T2 = T>
        auto operator<(ref<T2> const& r) const -> bool
        {
            static_assert(
                std::is_convertible_v<T2*, T*> || std::is_convertible_v<T*, T2*>, "Cannot compare references of non-convertible types."
            );
            return m_ptr < r.m_ptr;
        }

        template<typename T2 = T>
        auto operator==(T2 const* ptr) const -> bool
        {
            static_assert(std::is_convertible_v<T2*, T*>, "Cannot compare reference to pointer of non-convertible types.");
            return m_ptr == ptr;
        }

        template<typename T2 = T>
        auto operator!=(T2 const* ptr) const -> bool
        {
            static_assert(std::is_convertible_v<T2*, T*>, "Cannot compare reference to pointer of non-convertible types.");
            return m_ptr != ptr;
        }

        auto operator==(std::nullptr_t) const -> bool { return m_ptr == nullptr; }
        auto operator!=(std::nullptr_t) const -> bool { return m_ptr != nullptr; }
        auto operator<(std::nullptr_t) const -> bool { return m_ptr < nullptr; }

        auto operator->() const -> T* { return m_ptr; }
        auto operator*() const -> T& { return *m_ptr; }
        auto get() const -> T* { return m_ptr; }

        operator bool() const { return m_ptr != nullptr; }

        auto swap(ref& r) noexcept -> void
        {
            std::swap(m_ptr, r.m_ptr);
#if APRIL_ENABLE_REF_TRACKING
            std::swap(m_refId, r.m_refId);
#endif
        }

    private:
        inline auto incRef(Object const* object) -> void
        {
#if APRIL_ENABLE_REF_TRACKING
            object->incRef(m_refId);
#else
            object->incRef();
#endif
        }

        inline auto decRef(Object const* object) -> void
        {
#if APRIL_ENABLE_REF_TRACKING
            object->decRef(m_refId);
#else
            object->decRef();
#endif
        }

        T* m_ptr{nullptr};
#if APRIL_ENABLE_REF_TRACKING
        uint64_t m_refId{nextRefId()};
#endif

    private:
        template<typename T2>
        friend class ref;
    };

    template<class T, class... Args>
    auto make_ref(Args&&... args) -> ref<T>
    {
        return ref<T>(new T(std::forward<Args>(args)...));
    }

    template<class T, class U>
    auto static_ref_cast(ref<U> const& r) noexcept -> ref<T>
    {
        return ref<T>(static_cast<T*>(r.get()));
    }

    template<class T, class U>
    auto dynamic_ref_cast(ref<U> const& r) noexcept -> ref<T>
    {
        return ref<T>(dynamic_cast<T*>(r.get()));
    }

    // Breakable reference counting helper.
    template<typename T>
    class BreakableReference
    {
    public:
        BreakableReference(ref<T> const& r) : m_strongRef(r), m_weakRef(m_strongRef.get()) {}
        BreakableReference(ref<T>&& r) : m_strongRef(r), m_weakRef(m_strongRef.get()) {}

        BreakableReference() = delete;
        auto operator=(ref<T> const&) -> BreakableReference& = delete;
        auto operator=(ref<T>&&) -> BreakableReference& = delete;

        auto get() const -> T* { return m_weakRef; }
        auto operator->() const -> T* { return get(); }
        auto operator*() const -> T& { return *get(); }
        operator ref<T>() const { return ref<T>(get()); }
        operator T*() const { return get(); }
        operator bool() const { return get() != nullptr; }

        auto breakStrongReference() -> void { m_strongRef.reset(); }

    private:
        ref<T> m_strongRef;
        T* m_weakRef = nullptr;
    };
} // namespace april::core




template<typename T>
struct std::formatter<april::core::BreakableReference<T>> : std::formatter<void const*>
{
    auto format(april::core::BreakableReference<T> const& ref, std::format_context& ctx) const
    {
        return std::formatter<void const*>::format(ref.get(), ctx);
    }
};

namespace std
{
    template<typename T>
    auto swap(april::core::ref<T>& x, april::core::ref<T>& y) noexcept -> void
    {
        return x.swap(y);
    }

    template<typename T>
    struct hash<april::core::ref<T>>
    {
        constexpr auto operator()(april::core::ref<T> const& r) const -> size_t { return std::hash<T*>()(r.get()); }
    };
} // namespace std
