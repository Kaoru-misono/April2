#include "asset.hpp"

#include <core/serialization/binary-stream.hpp>
#include <core/error/assert.hpp>

namespace april
{
    auto Asset::serialize(Serializer& serializer) -> void
    {
        // Serialize header
         // 1. Write Header.
        auto header = AssetHeader{};

        serializer.write(header);

        auto const uuidBytes = m_handle.getBytes();
        serializer.writeBuffer(uuidBytes);

        serializer.write(m_type);

        serializer.write(m_assetPath);
        serializer.write(m_sourcePath);

        // Serialize asset-specific data in derived classes
    }

    auto Asset::deserialize(Deserializer& deserializer) -> bool
    {
        // Deserialize header
        auto header = AssetHeader{};
        deserializer.read(header);

        if ((header.magic != ASSET_MAGIC) || (header.version != ASSET_VERSION))
        {
            return false;
        }

        auto uuidBytes = std::vector<std::byte>{};
        deserializer.readBuffer(uuidBytes);
        AP_ASSERT(uuidBytes.size() == 16, "Invalid UUID size in asset.");
        m_handle = UUID(std::span<std::uint8_t const, 16>((std::uint8_t*) uuidBytes.data(), 16));

        deserializer.read(m_type);

        deserializer.read(m_assetPath);
        deserializer.read(m_sourcePath);

        // Deserialize asset-specific data in derived classes

        return true;
    }
}
