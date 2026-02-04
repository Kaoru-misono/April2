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

    inline auto to_json(nlohmann::json& j, MaterialParameters const& p) -> void
    {
        j["baseColorFactor"] = p.baseColorFactor;
        j["metallicFactor"] = p.metallicFactor;
        j["roughnessFactor"] = p.roughnessFactor;
        j["emissiveFactor"] = p.emissiveFactor;
        j["occlusionStrength"] = p.occlusionStrength;
        j["normalScale"] = p.normalScale;
        j["alphaCutoff"] = p.alphaCutoff;
        j["alphaMode"] = p.alphaMode;
        j["doubleSided"] = p.doubleSided;
    }

    inline auto from_json(nlohmann::json const& j, MaterialParameters& p) -> void
    {
        if (j.contains("baseColorFactor")) j.at("baseColorFactor").get_to(p.baseColorFactor);
        if (j.contains("metallicFactor")) p.metallicFactor = j.at("metallicFactor").get<float>();
        if (j.contains("roughnessFactor")) p.roughnessFactor = j.at("roughnessFactor").get<float>();
        if (j.contains("emissiveFactor")) j.at("emissiveFactor").get_to(p.emissiveFactor);
        if (j.contains("occlusionStrength")) p.occlusionStrength = j.at("occlusionStrength").get<float>();
        if (j.contains("normalScale")) p.normalScale = j.at("normalScale").get<float>();
        if (j.contains("alphaCutoff")) p.alphaCutoff = j.at("alphaCutoff").get<float>();
        if (j.contains("alphaMode")) p.alphaMode = j.at("alphaMode").get<std::string>();
        if (j.contains("doubleSided")) p.doubleSided = j.at("doubleSided").get<bool>();
    }

    struct TextureReference
    {
        std::string assetId{};  // UUID of TextureAsset
        int texCoord{0};        // UV channel (0 or 1)

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(TextureReference, assetId, texCoord)
    };

    struct MaterialTextures
    {
        std::optional<TextureReference> baseColorTexture{};
        std::optional<TextureReference> metallicRoughnessTexture{};
        std::optional<TextureReference> normalTexture{};
        std::optional<TextureReference> occlusionTexture{};
        std::optional<TextureReference> emissiveTexture{};
    };

    inline auto to_json(nlohmann::json& j, MaterialTextures const& t) -> void
    {
        j = nlohmann::json::object();
        if (t.baseColorTexture) j["baseColorTexture"] = *t.baseColorTexture;
        if (t.metallicRoughnessTexture) j["metallicRoughnessTexture"] = *t.metallicRoughnessTexture;
        if (t.normalTexture) j["normalTexture"] = *t.normalTexture;
        if (t.occlusionTexture) j["occlusionTexture"] = *t.occlusionTexture;
        if (t.emissiveTexture) j["emissiveTexture"] = *t.emissiveTexture;
    }

    inline auto from_json(nlohmann::json const& j, MaterialTextures& t) -> void
    {
        if (j.contains("baseColorTexture"))
            t.baseColorTexture = j.at("baseColorTexture").get<TextureReference>();
        if (j.contains("metallicRoughnessTexture"))
            t.metallicRoughnessTexture = j.at("metallicRoughnessTexture").get<TextureReference>();
        if (j.contains("normalTexture"))
            t.normalTexture = j.at("normalTexture").get<TextureReference>();
        if (j.contains("occlusionTexture"))
            t.occlusionTexture = j.at("occlusionTexture").get<TextureReference>();
        if (j.contains("emissiveTexture"))
            t.emissiveTexture = j.at("emissiveTexture").get<TextureReference>();
    }

    class MaterialAsset : public Asset
    {
    public:
        MaterialAsset() : Asset(AssetType::Material) {}

        MaterialParameters parameters{};
        MaterialTextures textures{};

        auto serializeJson(nlohmann::json& outJson) -> void override;
        auto deserializeJson(nlohmann::json const& inJson) -> bool override;
        [[nodiscard]] auto computeDDCKey() const -> std::string;
    };
}
