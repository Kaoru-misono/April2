#include "material-importer.hpp"

#include "../material-asset.hpp"
#include "../ddc/ddc-key.hpp"
#include "../ddc/ddc-utils.hpp"

#include <core/log/logger.hpp>

#include <nlohmann/json.hpp>
#include <cstring>

namespace april::asset
{
    auto MaterialImporter::import(ImportContext const& context) -> ImportResult
    {
        context.deps.deps.clear();

        auto const& asset = static_cast<MaterialAsset const&>(context.asset);

        for (auto const& ref : asset.getReferences())
        {
            context.deps.addStrong(ref);
        }

        auto settingsJson = nlohmann::json{};
        settingsJson["parameters"] = asset.parameters;
        settingsJson["textures"] = asset.textures;

        auto settingsHash = hashJson(settingsJson);
        auto depsHash = hashDependencies(context.deps.deps);

        constexpr auto kMaterialToolchainTag = "material-json@1";
        auto key = buildDdcKey(FingerprintInput{
            "MT",
            asset.getHandle().toString(),
            std::string{id()},
            version(),
            kMaterialToolchainTag,
            "",
            settingsHash,
            depsHash,
            context.target
        });

        auto result = ImportResult{};
        if (!context.forceReimport && context.ddc.exists(key))
        {
            result.producedKeys.push_back(key);
            return result;
        }

        auto payload = settingsJson.dump();
        auto bytes = std::vector<std::byte>(payload.size());
        std::memcpy(bytes.data(), payload.data(), payload.size());

        auto value = DdcValue{};
        value.bytes = std::move(bytes);
        context.ddc.put(key, value);

        AP_INFO("[MaterialImporter] Cooked material: {} ({} bytes)", asset.getHandle().toString(), payload.size());

        result.producedKeys.push_back(key);
        return result;
    }
} // namespace april::asset
