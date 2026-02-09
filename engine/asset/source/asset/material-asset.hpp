#pragma once

#include "asset.hpp"
#include <optional>
#include <core/math/json.hpp>
#include <core/math/type.hpp>

namespace april::asset
{
    struct MaterialParameters
    {
        // PBR Base Parameters
        float4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};  // RGBA
        float metallicFactor{1.0f};
        float roughnessFactor{1.0f};
        float3 emissiveFactor{0.0f, 0.0f, 0.0f};
        float occlusionStrength{1.0f};
        float normalScale{1.0f};
        float alphaCutoff{0.5f};

        // Render State
        std::string alphaMode{"OPAQUE"};  // "OPAQUE", "MASK", "BLEND"
        bool doubleSided{false};
    };

    auto to_json(nlohmann::json& j, MaterialParameters const& p) -> void;
    auto from_json(nlohmann::json const& j, MaterialParameters& p) -> void;

    struct TextureReference
    {
        AssetRef asset{};
        int texCoord{0};        // UV channel (0 or 1)
    };

    auto to_json(nlohmann::json& j, TextureReference const& ref) -> void;
    auto from_json(nlohmann::json const& j, TextureReference& ref) -> void;

    struct MaterialTextures
    {
        std::optional<TextureReference> baseColorTexture{};
        std::optional<TextureReference> metallicRoughnessTexture{};
        std::optional<TextureReference> normalTexture{};
        std::optional<TextureReference> occlusionTexture{};
        std::optional<TextureReference> emissiveTexture{};
    };

    auto to_json(nlohmann::json& j, MaterialTextures const& t) -> void;
    auto from_json(nlohmann::json const& j, MaterialTextures& t) -> void;

    class MaterialAsset : public Asset
    {
    public:
        MaterialAsset() : Asset(AssetType::Material) {}

        // Material type name (e.g., "Standard", "Unlit") - explicit metadata, not path-derived.
        std::string materialType{"Standard"};

        MaterialParameters parameters{};
        MaterialTextures textures{};

        auto serializeJson(nlohmann::json& outJson) -> void override;
        auto deserializeJson(nlohmann::json const& inJson) -> bool override;

    private:
        auto rebuildReferences() -> void;
    };
}
