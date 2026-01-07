#pragma once

#include <uuid.h>
#include <string>
#include <functional>

namespace april
{
    static constexpr uuids::uuid s_engineNamespace{uuids::uuid::from_string("00000000-0000-0000-0000-000000000000").value()};

    struct UUID
    {
        // Default constructor: Generates a new random v4 UUID.
        UUID()
            : m_uuid{}
        {
            static std::random_device rd{};
            static std::mt19937 gen{rd()};
            static uuids::uuid_random_generator uuidGen(gen);

            m_uuid = uuidGen();
        }

        // Create from string (e.g. from serialization).
        // Format: "47183823-2574-4bfd-b411-99ed177d3e43"
        UUID(std::string const& str)
        {
            auto id = uuids::uuid::from_string(str);
            if (id.has_value())
            {
                m_uuid = id.value();
            }
        }

        UUID(std::span<std::uint8_t const, 16> bytes)
            : m_uuid{bytes.begin(), bytes.end()}
        {}

        // Copy constructor.
        UUID(UUID const&) = default;

        // ToString helper for serialization/logging.
        [[nodiscard]] auto toString() const -> std::string
        {
            return uuids::to_string(m_uuid);
        }

        // Operators for map/set comparisons.
        auto operator == (UUID const& other) const -> bool = default;

        // Allow access to underlying data if strictly necessary.
        [[nodiscard]] auto getNative() const -> uuids::uuid const& { return m_uuid; }
        [[nodiscard]] auto getBytes() const -> std::span<std::byte const, 16> { return m_uuid.as_bytes(); }

    private:
        uuids::uuid m_uuid{};
    };
}

// Enable hashing for std::unordered_map.
namespace std
{
    template <>
    struct hash<april::UUID>
    {
        auto operator () (april::UUID const& id) const -> size_t
        {
            // Use stduuid's built-in hash function.
            return std::hash<uuids::uuid>{}(id.getNative());
        }
    };
}
