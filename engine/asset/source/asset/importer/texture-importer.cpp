#include "texture-importer.hpp"

#include "../blob-header.hpp"
#include "../texture-asset.hpp"
#include "../ddc/ddc-key.hpp"
#include "../ddc/ddc-utils.hpp"

#include <core/log/logger.hpp>

#include <stb/stb_image.h>

#include <cstring>
#include <nlohmann/json.hpp>
#include <vector>

namespace april::asset
{
    namespace
    {
        constexpr auto kTextureToolchainTag = "stb_image@unknown|texblob@1";

        auto compileTexture(std::string const& sourcePath, TextureImportSettings const& settings) -> std::vector<std::byte>
        {
            auto width = 0;
            auto height = 0;
            auto channels = 0;
            auto constexpr desiredChannels = 4;

            auto* pixels = stbi_load(sourcePath.c_str(), &width, &height, &channels, desiredChannels);
            if (!pixels)
            {
                AP_ERROR("[TextureImporter] Failed to load image: {} - {}", sourcePath, stbi_failure_reason());
                return {};
            }

            auto const dataSize = static_cast<uint64_t>(width) * height * desiredChannels;

            auto header = TextureHeader{};
            header.width = static_cast<uint32_t>(width);
            header.height = static_cast<uint32_t>(height);
            header.channels = desiredChannels;
            header.format = settings.sRGB ? PixelFormat::RGBA8UnormSrgb : PixelFormat::RGBA8Unorm;
            header.mipLevels = 1;
            header.flags = settings.sRGB ? 1 : 0;
            header.dataSize = dataSize;

            auto blob = std::vector<std::byte>(sizeof(TextureHeader) + dataSize);
            std::memcpy(blob.data(), &header, sizeof(TextureHeader));
            std::memcpy(blob.data() + sizeof(TextureHeader), pixels, dataSize);

            stbi_image_free(pixels);

            AP_INFO("[TextureImporter] Compiled texture: {}x{} {} channels, {} mips, {} bytes",
                    header.width, header.height, header.channels, header.mipLevels, blob.size());

            return blob;
        }
    }

    auto TextureImporter::import(ImportContext const& context) -> ImportResult
    {
        context.deps.deps.clear();

        auto const& asset = static_cast<TextureAsset const&>(context.asset);
        auto sourcePath = context.sourcePath.empty() ? asset.getSourcePath() : context.sourcePath;

        auto settingsJson = nlohmann::json{};
        settingsJson["settings"] = asset.m_settings;
        auto settingsHash = hashJson(settingsJson);

        auto sourceHash = hashFileContents(sourcePath);
        auto depsHash = hashDependencies(context.deps.deps);

        auto key = buildDdcKey(FingerprintInput{
            "TX",
            asset.getHandle().toString(),
            std::string{id()},
            version(),
            kTextureToolchainTag,
            sourceHash,
            settingsHash,
            depsHash,
            context.target
        });

        auto result = ImportResult{};

        if (asset.m_settings.generateMips)
        {
            result.warnings.push_back("generateMips requested but mip generation is not implemented");
            AP_WARN("[TextureImporter] generateMips requested but not implemented");
        }

        if (!asset.m_settings.compression.empty() && asset.m_settings.compression != "RGBA8")
        {
            result.warnings.push_back("compression setting is not implemented yet");
            AP_WARN("[TextureImporter] compression '{}' not implemented", asset.m_settings.compression);
        }

        if (asset.m_settings.brightness != 1.0f)
        {
            result.warnings.push_back("brightness setting is not implemented yet");
            AP_WARN("[TextureImporter] brightness {} not implemented", asset.m_settings.brightness);
        }

        if (!context.forceReimport && context.ddc.exists(key))
        {
            result.producedKeys.push_back(key);
            return result;
        }

        auto blob = compileTexture(sourcePath, asset.m_settings);
        if (blob.empty())
        {
            result.errors.push_back("Texture compilation failed");
            return result;
        }

        auto value = DdcValue{};
        value.bytes = std::move(blob);
        context.ddc.put(key, value);

        result.producedKeys.push_back(key);
        return result;
    }
} // namespace april::asset
