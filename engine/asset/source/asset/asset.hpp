#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <core/tools/uuid.hpp>

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
        [[nodiscard]] auto getHandle() const -> UUID { return m_handle; }
        [[nodiscard]] auto getType() const -> AssetType { return m_type; }

        [[nodiscard]] auto getSourcePath() const -> std::string const& { return m_sourcePath; }
        auto setSourcePath(std::string_view path) -> void { m_sourcePath = path; }

        virtual auto serializeJson(nlohmann::json& outJson) -> void;
        virtual auto deserializeJson(nlohmann::json const& inJson) -> bool;

    protected:
        Asset(AssetType type)
            : m_handle{UUID()}
            , m_type{type}
        {}

        UUID m_handle{};
        AssetType m_type{AssetType::None};

        std::string m_sourcePath{};
    };
}
