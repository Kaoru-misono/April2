#pragma once
#pragma once

#include <string>
#include "asset-ref.hpp"

#include <nlohmann/json.hpp>
#include <vector>
#include <string_view>

namespace april::asset
{
    enum struct AssetType : uint8_t
    {
        None,
        Texture,
        Mesh,
        Shader,
        Material
    };

    NLOHMANN_JSON_SERIALIZE_ENUM( AssetType, {
        {AssetType::None, "None"},
        {AssetType::Texture, "Texture"},
        {AssetType::Mesh, "Mesh"},
        {AssetType::Shader, "Shader"},
        {AssetType::Material, "Material"},
    })

    class Asset
    {
    public:
        using UUID = core::UUID;
        virtual ~Asset() = default;

        // --- Getters ---
        [[nodiscard]] auto getHandle() const -> UUID;
        [[nodiscard]] auto getType() const -> AssetType;

        [[nodiscard]] auto getSourcePath() const -> std::string const&;
        auto setSourcePath(std::string_view path) -> void;

        [[nodiscard]] auto getAssetPath() const -> std::string const&;
        auto setAssetPath(std::string_view path) -> void;

        [[nodiscard]] auto getImporterId() const -> std::string const&;
        [[nodiscard]] auto getImporterVersion() const -> int;
        auto setImporter(std::string importerId, int importerVersion) -> void;

        [[nodiscard]] auto getReferences() const -> std::vector<AssetRef> const&;
        auto setReferences(std::vector<AssetRef> references) -> void;

        virtual auto serializeJson(nlohmann::json& outJson) -> void;
        virtual auto deserializeJson(nlohmann::json const& inJson) -> bool;

    protected:
        Asset(AssetType type);

        UUID m_handle{};
        AssetType m_type{AssetType::None};

        std::string m_sourcePath{};
        std::string m_assetPath{};
        std::string m_importerId{};
        int m_importerVersion = 0;
        std::vector<AssetRef> m_references{};
    };
}
