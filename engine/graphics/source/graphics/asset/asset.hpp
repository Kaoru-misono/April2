#pragma once

#include "uuid.hpp"
#include <string>

namespace april
{
    static constexpr uint32_t ASSET_MAGIC = 0x4C525041;
    static constexpr uint32_t ASSET_VERSION = 1;

    enum struct AssetType: uint8_t
    {
        None,
        Texture,
        Mesh,
        Shader,
        Material
    };

    // Forward declare your serialization helper
    struct Serializer;
    struct Deserializer;

    // This struct must be tightly packed or standard layout for binary IO.
    struct AssetHeader
    {
        uint32_t magic{ASSET_MAGIC};
        uint32_t version{ASSET_VERSION};
    };

    struct IAsset
    {
        virtual ~IAsset() = default;

        [[nodiscard]] virtual auto getHandle() const -> UUID = 0;
        [[nodiscard]] virtual auto getType() const -> AssetType = 0;
        [[nodiscard]] virtual auto getAssetPath() const -> std::string const& = 0;

        // Used when editor needs to know where this asset came from
        [[nodiscard]] virtual auto getSourcePath() const -> std::string const& = 0;

        // Virtual methods for serialization
        virtual auto serialize(Serializer& serializer) -> void = 0;
        virtual auto deserialize(Deserializer& deserializer) -> bool = 0;
    };

    struct Asset: IAsset
    {
        virtual ~Asset() = default;

        auto getHandle() const -> UUID override { return m_handle; }
        auto getType() const -> AssetType override { return m_type; }
        auto getAssetPath() const -> std::string const& override { return m_assetPath; }

        // Used when editor needs to know where this asset came from
        auto getSourcePath() const -> std::string const& override { return m_sourcePath; }

        auto serialize(Serializer& serializer) -> void override;
        auto deserialize(Deserializer& deserializer) -> bool override;

    protected:
        Asset(AssetType type)
            : m_handle{UUID()} // Generate new random ID
            , m_type{type}
        {}

        UUID m_handle{};
        AssetType m_type{AssetType::None};
        std::string m_sourcePath{};
        std::string m_assetPath{};
    };
}
