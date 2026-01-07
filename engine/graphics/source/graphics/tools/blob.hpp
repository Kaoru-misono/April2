#pragma once

#include <slang-rhi.h>
#include <vector>

namespace april
{
    /**
    * A minimal implementation of ISlangBlob that owns its memory.
    * Does not use atomic reference counting for simplicity.
    */
    struct SimpleBlob: public ISlangBlob
    {
    public:
        // ------------------------------------------------------------------------
        // ISlangUnknown Implementation
        // ------------------------------------------------------------------------

        SLANG_NO_THROW auto SLANG_MCALL queryInterface(SlangUUID const& uuid, void** outObject) -> SlangResult override
        {
            (void) uuid;
            (void) outObject;

            return SLANG_E_NO_INTERFACE;
        }

        SLANG_NO_THROW auto SLANG_MCALL addRef() -> uint32_t override
        {
            m_refCount++;
            return m_refCount;
        }

        SLANG_NO_THROW auto SLANG_MCALL release() -> uint32_t override
        {
            m_refCount--;
            auto const currentRefs = m_refCount;

            if (currentRefs == 0)
            {
                delete this;
            }

            return currentRefs;
        }

        // ------------------------------------------------------------------------
        // ISlangBlob Implementation
        // ------------------------------------------------------------------------

        SLANG_NO_THROW auto SLANG_MCALL getBufferPointer() -> void const* override
        {
            return m_data.data();
        }

        SLANG_NO_THROW auto SLANG_MCALL getBufferSize() -> size_t override
        {
            return m_data.size();
        }

        // ------------------------------------------------------------------------
        // Factory Method
        // ------------------------------------------------------------------------

        /**
        * Creates a new SimpleBlob and copies the data into it.
        */
        static auto create(void const* data, size_t size) -> rhi::ComPtr<ISlangBlob>
        {
            return rhi::ComPtr<ISlangBlob>(new SimpleBlob(data, size));
        }

    private:
        // ------------------------------------------------------------------------
        // Internal Logic
        // ------------------------------------------------------------------------

        explicit SimpleBlob(const void* data, size_t size)
        {
            m_data.resize(size);

            if (data && size > 0)
            {
                std::memcpy(m_data.data(), data, size);
            }
        }

        // Destructor is private to enforce lifecycle management via release().
        virtual ~SimpleBlob() = default;

        // ------------------------------------------------------------------------
        // Member Variables
        // ------------------------------------------------------------------------

        std::vector<uint8_t> m_data{};
        uint32_t m_refCount{0};
    };

} // namespace rhi
