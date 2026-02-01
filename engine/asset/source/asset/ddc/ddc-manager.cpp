#include "ddc-manager.hpp"
#include "../blob-header.hpp"

#include <core/log/logger.hpp>
#include <stb/stb_image.h>

#include <fstream>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace april::asset
{
    DDCManager::DDCManager(std::filesystem::path cacheRoot)
        : m_cacheRoot{std::move(cacheRoot)}
    {
        if (!std::filesystem::exists(m_cacheRoot))
        {
            std::filesystem::create_directories(m_cacheRoot);
        }
    }

    auto DDCManager::getOrCompileTexture(TextureAsset const& asset) -> std::vector<std::byte>
    {
        auto key = asset.computeDDCKey();
        auto subDir = key.substr(0, 2);
        auto cacheDir = m_cacheRoot / subDir;
        auto cacheFile = cacheDir / (key + ".bin");

        if (std::filesystem::exists(cacheFile))
        {
            AP_INFO("[DDC] Cache hit: {}", key);
            return loadFile(cacheFile);
        }

        if (!std::filesystem::exists(cacheDir))
        {
            std::filesystem::create_directories(cacheDir);
        }

        AP_INFO("[DDC] Compiling texture: {}", asset.getSourcePath());
        auto blob = compileInternal(asset);

        if (!blob.empty())
        {
            saveFile(cacheFile, blob);
        }

        return blob;
    }

    auto DDCManager::compileInternal(TextureAsset const& asset) -> std::vector<std::byte>
    {
        auto const& sourcePath = asset.getSourcePath();
        auto const& settings = asset.m_settings;

        // Load image using stb_image
        auto width = 0;
        auto height = 0;
        auto channels = 0;

        // Request 4 channels (RGBA) for consistency
        auto constexpr desiredChannels = 4;
        auto* pixels = stbi_load(sourcePath.c_str(), &width, &height, &channels, desiredChannels);

        if (!pixels)
        {
            AP_ERROR("[DDC] Failed to load image: {} - {}", sourcePath, stbi_failure_reason());
            return {};
        }

        // Calculate data size
        auto const dataSize = static_cast<uint64_t>(width) * height * desiredChannels;

        // Build texture header
        auto header = TextureHeader{};
        header.width = static_cast<uint32_t>(width);
        header.height = static_cast<uint32_t>(height);
        header.channels = desiredChannels;
        header.format = settings.sRGB ? PixelFormat::RGBA8UnormSrgb : PixelFormat::RGBA8Unorm;
        header.mipLevels = settings.generateMips ? calculateMipLevels(width, height) : 1;
        header.flags = settings.sRGB ? 1 : 0; // Flag bit 0 = sRGB
        header.dataSize = dataSize;

        // Create blob: header + pixel data
        auto blob = std::vector<std::byte>(sizeof(TextureHeader) + dataSize);

        // Copy header
        std::memcpy(blob.data(), &header, sizeof(TextureHeader));

        // Copy pixel data
        std::memcpy(blob.data() + sizeof(TextureHeader), pixels, dataSize);

        // Free stb_image memory
        stbi_image_free(pixels);

        AP_INFO("[DDC] Compiled texture: {}x{} {} channels, {} mips, {} bytes",
                header.width, header.height, header.channels, header.mipLevels, blob.size());

        return blob;
    }

    auto DDCManager::calculateMipLevels(int width, int height) -> uint32_t
    {
        auto maxDim = std::max(width, height);
        auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(maxDim))) + 1;
        return mipLevels;
    }

    auto DDCManager::loadFile(std::filesystem::path const& path) -> std::vector<std::byte>
    {
        auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
        if (!file.is_open())
        {
            return {};
        }

        auto size = file.tellg();
        auto buffer = std::vector<std::byte>(size);
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(buffer.data()), size);
        return buffer;
    }

    auto DDCManager::saveFile(std::filesystem::path const& path, std::vector<std::byte> const& data) -> void
    {
        auto file = std::ofstream{path, std::ios::binary};
        if (file.is_open())
        {
            file.write(reinterpret_cast<char const*>(data.data()), data.size());
        }
    }

} // namespace april::asset
