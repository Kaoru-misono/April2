#include "texture-importer.hpp"

#include "../blob-header.hpp"
#include "../texture-asset.hpp"
#include "../ddc/ddc-key.hpp"
#include "../ddc/ddc-utils.hpp"

#include <core/log/logger.hpp>

#include <stb/stb_image.h>

#include <cstring>
#include <algorithm>
#include <span>
#include <nlohmann/json.hpp>
#include <vector>

namespace april::asset
{
    namespace
    {
        constexpr auto kTextureToolchainTag = "stb_image@unknown|texblob@1";

        auto calculateMipLevels(int width, int height) -> uint32_t
        {
            auto levels = uint32_t{1};
            while (width > 1 || height > 1)
            {
                width = std::max(1, width / 2);
                height = std::max(1, height / 2);
                ++levels;
            }
            return levels;
        }

        auto downsampleRgba8(std::span<uint8_t const> src, int width, int height) -> std::vector<uint8_t>
        {
            auto const nextWidth = std::max(1, width / 2);
            auto const nextHeight = std::max(1, height / 2);
            auto result = std::vector<uint8_t>(static_cast<size_t>(nextWidth * nextHeight * 4));

            for (auto y = 0; y < nextHeight; ++y)
            {
                for (auto x = 0; x < nextWidth; ++x)
                {
                    auto const srcX = x * 2;
                    auto const srcY = y * 2;

                    auto const x1 = std::min(srcX + 1, width - 1);
                    auto const y1 = std::min(srcY + 1, height - 1);

                    auto const idx00 = (srcY * width + srcX) * 4;
                    auto const idx10 = (srcY * width + x1) * 4;
                    auto const idx01 = (y1 * width + srcX) * 4;
                    auto const idx11 = (y1 * width + x1) * 4;

                    auto const dstIndex = (y * nextWidth + x) * 4;

                    for (auto c = 0; c < 4; ++c)
                    {
                        auto sum = static_cast<uint32_t>(src[idx00 + c]) +
                                   static_cast<uint32_t>(src[idx10 + c]) +
                                   static_cast<uint32_t>(src[idx01 + c]) +
                                   static_cast<uint32_t>(src[idx11 + c]);
                        result[static_cast<size_t>(dstIndex + c)] = static_cast<uint8_t>(sum / 4);
                    }
                }
            }

            return result;
        }

        auto appendBytes(std::vector<std::byte>& dst, std::span<uint8_t const> src) -> void
        {
            auto const offset = dst.size();
            dst.resize(offset + src.size());
            if (!src.empty())
            {
                std::memcpy(dst.data() + offset, src.data(), src.size());
            }
        }

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

            auto const dataSize = static_cast<size_t>(width) * height * desiredChannels;
            auto const mipLevels = settings.generateMips ? calculateMipLevels(width, height) : 1u;
            auto mipBytes = std::vector<std::byte>{};
            mipBytes.reserve(dataSize);

            auto levelPixels = std::vector<uint8_t>(pixels, pixels + dataSize);
            appendBytes(mipBytes, levelPixels);

            stbi_image_free(pixels);

            if (settings.generateMips)
            {
                auto currentWidth = width;
                auto currentHeight = height;
                while (currentWidth > 1 || currentHeight > 1)
                {
                    auto nextPixels = downsampleRgba8(levelPixels, currentWidth, currentHeight);
                    appendBytes(mipBytes, nextPixels);
                    levelPixels = std::move(nextPixels);
                    currentWidth = std::max(1, currentWidth / 2);
                    currentHeight = std::max(1, currentHeight / 2);
                }
            }

            auto header = TextureHeader{};
            header.width = static_cast<uint32_t>(width);
            header.height = static_cast<uint32_t>(height);
            header.channels = desiredChannels;
            header.format = settings.sRGB ? PixelFormat::RGBA8UnormSrgb : PixelFormat::RGBA8Unorm;
            header.mipLevels = mipLevels;
            header.flags = settings.sRGB ? 1 : 0;
            header.dataSize = mipBytes.size();

            auto blob = std::vector<std::byte>(sizeof(TextureHeader) + header.dataSize);
            std::memcpy(blob.data(), &header, sizeof(TextureHeader));
            if (!mipBytes.empty())
            {
                std::memcpy(blob.data() + sizeof(TextureHeader), mipBytes.data(), mipBytes.size());
            }

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
