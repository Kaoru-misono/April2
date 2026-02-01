#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <cstring>

#include <asset/asset.hpp>
#include <asset/texture-asset.hpp>
#include <asset/blob-header.hpp>
#include <asset/ddc/ddc-manager.hpp>
#include <asset/asset-manager.hpp>

namespace fs = std::filesystem;

// Helper: Create a minimal valid PNG file (1x1 red RGBA pixel)
auto createMinimalPNG(std::string const& path) -> void
{
    // Valid 1x1 RGBA PNG (red pixel) - 70 bytes
    static constexpr unsigned char pngData[] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
        0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
        0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4, 0x89, 0x00, 0x00, 0x00,
        0x0D, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0x63, 0xF8, 0xCF, 0xC0, 0xF0,
        0x1F, 0x00, 0x05, 0x00, 0x01, 0xFF, 0x56, 0xC7, 0x2F, 0x0D, 0x00, 0x00,
        0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
    };

    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<char const*>(pngData), sizeof(pngData));
}

// Helper: Create a 2x2 RGBA PNG for more thorough testing
auto create2x2PNG(std::string const& path) -> void
{
    // Valid 2x2 RGBA PNG (red, green, blue, white pixels) - 75 bytes
    static constexpr unsigned char pngData[] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
        0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
        0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xB6, 0x0D, 0x24, 0x00, 0x00, 0x00,
        0x12, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0x63, 0xF8, 0xCF, 0xC0, 0xF0,
        0x1F, 0x0C, 0x81, 0x34, 0x18, 0x00, 0x00, 0x49, 0xC8, 0x09, 0xF7, 0x03,
        0xD9, 0x64, 0xF1, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,
        0x42, 0x60, 0x82,
    };

    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<char const*>(pngData), sizeof(pngData));
}

// Helper: Create a dummy text file (for error testing)
auto createDummyFile(std::string const& path, std::string const& content) -> void
{
    std::ofstream f(path);
    f << content;
}

TEST_SUITE("Asset System - Stage 3 & 4")
{
    TEST_CASE("TextureHeader - Binary Layout")
    {
        using namespace april::asset;

        SUBCASE("Header size is fixed at 40 bytes")
        {
            CHECK(sizeof(TextureHeader) == 40);
        }

        SUBCASE("Header magic value is correct")
        {
            auto header = TextureHeader{};
            CHECK(header.magic == 0x41505458); // "APTX"
        }

        SUBCASE("Default header is invalid (zero dimensions)")
        {
            auto header = TextureHeader{};
            CHECK_FALSE(header.isValid());
        }

        SUBCASE("Header with valid dimensions is valid")
        {
            auto header = TextureHeader{};
            header.width = 64;
            header.height = 64;
            CHECK(header.isValid());
        }

        SUBCASE("Header with wrong magic is invalid")
        {
            auto header = TextureHeader{};
            header.width = 64;
            header.height = 64;
            header.magic = 0x12345678;
            CHECK_FALSE(header.isValid());
        }
    }

    TEST_CASE("TexturePayload - Validation")
    {
        using namespace april::asset;

        SUBCASE("Empty payload is invalid")
        {
            auto payload = TexturePayload{};
            CHECK_FALSE(payload.isValid());
        }

        SUBCASE("Payload with valid header but empty data is invalid")
        {
            auto payload = TexturePayload{};
            payload.header.width = 64;
            payload.header.height = 64;
            CHECK_FALSE(payload.isValid());
        }

        SUBCASE("Payload with valid header and data is valid")
        {
            auto data = std::vector<std::byte>(256);
            auto payload = TexturePayload{};
            payload.header.width = 64;
            payload.header.height = 64;
            payload.pixelData = std::span<std::byte const>{data};
            CHECK(payload.isValid());
        }
    }

    TEST_CASE("TextureAsset - JSON Serialization")
    {
        using namespace april::asset;

        auto const testDir = std::string{"TestAssets_Serialize"};
        auto const srcFile = testDir + "/test.png";

        fs::create_directories(testDir);
        createMinimalPNG(srcFile);

        SUBCASE("Serialize and deserialize preserves data")
        {
            auto asset = TextureAsset{};
            asset.setSourcePath(srcFile);
            asset.m_settings.compression = "BC7";
            asset.m_settings.sRGB = true;
            asset.m_settings.generateMips = true;
            asset.m_settings.brightness = 1.5f;

            auto json = nlohmann::json{};
            asset.serializeJson(json);

            CHECK(json["source_path"] == srcFile);
            CHECK(json["type"] == "Texture");
            CHECK(json["settings"]["compression"] == "BC7");
            CHECK(json["settings"]["sRGB"] == true);
            CHECK(json["settings"]["generateMips"] == true);
            CHECK(json["settings"]["brightness"] == doctest::Approx(1.5f));

            auto asset2 = TextureAsset{};
            CHECK(asset2.deserializeJson(json));
            CHECK(asset2.getSourcePath() == srcFile);
            CHECK(asset2.m_settings.compression == "BC7");
            CHECK(asset2.m_settings.sRGB == true);
            CHECK(asset2.m_settings.generateMips == true);
            CHECK(asset2.m_settings.brightness == doctest::Approx(1.5f));
        }

        SUBCASE("Different settings produce different DDC keys")
        {
            auto asset1 = TextureAsset{};
            asset1.setSourcePath(srcFile);
            asset1.m_settings.sRGB = true;

            auto asset2 = TextureAsset{};
            asset2.setSourcePath(srcFile);
            asset2.m_settings.sRGB = false;

            auto key1 = asset1.computeDDCKey();
            auto key2 = asset2.computeDDCKey();

            CHECK(key1.length() == 40); // SHA1 hex
            CHECK(key2.length() == 40);
            CHECK(key1 != key2);
        }

        fs::remove_all(testDir);
    }

    TEST_CASE("DDCManager - Texture Compilation")
    {
        using namespace april::asset;

        auto const testDir = std::string{"TestAssets_DDC"};
        auto const cacheDir = std::string{"TestCache_DDC"};
        auto const srcFile = testDir + "/texture.png";

        fs::remove_all(testDir);
        fs::remove_all(cacheDir);
        fs::create_directories(testDir);

        createMinimalPNG(srcFile);

        SUBCASE("Compiles valid PNG to binary blob")
        {
            auto manager = DDCManager{cacheDir};
            auto asset = TextureAsset{};
            asset.setSourcePath(srcFile);
            asset.m_settings.sRGB = false;
            asset.m_settings.generateMips = false;

            auto blob = manager.getOrCompileTexture(asset);

            CHECK(blob.size() > sizeof(TextureHeader));

            // Parse header from blob
            auto header = TextureHeader{};
            std::memcpy(&header, blob.data(), sizeof(TextureHeader));

            CHECK(header.isValid());
            CHECK(header.width == 1);
            CHECK(header.height == 1);
            CHECK(header.channels == 4); // Always RGBA
            CHECK(header.format == PixelFormat::RGBA8Unorm);
            CHECK(header.mipLevels == 1);
            CHECK(header.dataSize == 4); // 1x1 RGBA = 4 bytes
        }

        SUBCASE("sRGB setting affects format")
        {
            auto manager = DDCManager{cacheDir};

            auto assetLinear = TextureAsset{};
            assetLinear.setSourcePath(srcFile);
            assetLinear.m_settings.sRGB = false;

            auto assetSrgb = TextureAsset{};
            assetSrgb.setSourcePath(srcFile);
            assetSrgb.m_settings.sRGB = true;

            auto blobLinear = manager.getOrCompileTexture(assetLinear);
            auto blobSrgb = manager.getOrCompileTexture(assetSrgb);

            auto headerLinear = TextureHeader{};
            auto headerSrgb = TextureHeader{};
            std::memcpy(&headerLinear, blobLinear.data(), sizeof(TextureHeader));
            std::memcpy(&headerSrgb, blobSrgb.data(), sizeof(TextureHeader));

            CHECK(headerLinear.format == PixelFormat::RGBA8Unorm);
            CHECK(headerSrgb.format == PixelFormat::RGBA8UnormSrgb);
        }

        SUBCASE("Cache hit returns same data")
        {
            auto manager = DDCManager{cacheDir};
            auto asset = TextureAsset{};
            asset.setSourcePath(srcFile);

            auto blob1 = manager.getOrCompileTexture(asset);
            auto blob2 = manager.getOrCompileTexture(asset);

            CHECK(blob1 == blob2);
        }

        SUBCASE("Cache file is created")
        {
            auto manager = DDCManager{cacheDir};
            auto asset = TextureAsset{};
            asset.setSourcePath(srcFile);

            manager.getOrCompileTexture(asset);

            auto key = asset.computeDDCKey();
            auto subDir = key.substr(0, 2);
            auto cacheFile = fs::path(cacheDir) / subDir / (key + ".bin");

            CHECK(fs::exists(cacheFile));
        }

        SUBCASE("Invalid image returns empty blob")
        {
            auto manager = DDCManager{cacheDir};
            auto asset = TextureAsset{};
            asset.setSourcePath(testDir + "/nonexistent.png");

            auto blob = manager.getOrCompileTexture(asset);
            CHECK(blob.empty());
        }

        fs::remove_all(testDir);
        fs::remove_all(cacheDir);
    }

    TEST_CASE("AssetManager - Texture Loading")
    {
        using namespace april::asset;

        auto const testDir = std::string{"TestAssets_Manager"};
        auto const cacheDir = std::string{"TestCache_Manager"};
        auto const srcFile = testDir + "/hero.png";
        auto const assetFile = testDir + "/hero.asset";

        fs::remove_all(testDir);
        fs::remove_all(cacheDir);
        fs::create_directories(testDir);

        createMinimalPNG(srcFile);

        // Create .asset file
        {
            auto asset = TextureAsset{};
            asset.setSourcePath(srcFile);
            asset.m_settings.sRGB = true;

            auto json = nlohmann::json{};
            asset.serializeJson(json);

            std::ofstream file(assetFile);
            file << json.dump(2);
        }

        SUBCASE("Load asset from file")
        {
            auto manager = AssetManager{testDir, cacheDir};
            auto asset = manager.loadAsset<TextureAsset>(assetFile);

            REQUIRE(asset != nullptr);
            CHECK(asset->getSourcePath() == srcFile);
            CHECK(asset->getType() == AssetType::Texture);
            CHECK(asset->m_settings.sRGB == true);
        }

        SUBCASE("Get texture data returns valid payload")
        {
            auto manager = AssetManager{testDir, cacheDir};
            auto asset = manager.loadAsset<TextureAsset>(assetFile);
            REQUIRE(asset != nullptr);

            auto blob = std::vector<std::byte>{};
            auto payload = manager.getTextureData(*asset, blob);

            CHECK(payload.isValid());
            CHECK(payload.header.width == 1);
            CHECK(payload.header.height == 1);
            CHECK(payload.header.channels == 4);
            CHECK(payload.header.format == PixelFormat::RGBA8UnormSrgb);
            CHECK(payload.pixelData.size() == 4);
        }

        SUBCASE("Register and retrieve asset by UUID")
        {
            auto manager = AssetManager{testDir, cacheDir};
            auto asset = manager.loadAsset<TextureAsset>(assetFile);
            REQUIRE(asset != nullptr);

            auto uuid = asset->getHandle();
            manager.registerAssetPath(uuid, assetFile);

            auto retrieved = manager.getAsset<TextureAsset>(uuid);
            REQUIRE(retrieved != nullptr);
            CHECK(retrieved->getHandle() == uuid);
        }

        fs::remove_all(testDir);
        fs::remove_all(cacheDir);
    }

    TEST_CASE("Mip Level Calculation")
    {
        using namespace april::asset;

        auto const testDir = std::string{"TestAssets_Mips"};
        auto const cacheDir = std::string{"TestCache_Mips"};

        fs::remove_all(testDir);
        fs::remove_all(cacheDir);
        fs::create_directories(testDir);

        SUBCASE("1x1 texture has 1 mip level")
        {
            auto srcFile = testDir + "/1x1.png";
            createMinimalPNG(srcFile);

            auto manager = DDCManager{cacheDir};
            auto asset = TextureAsset{};
            asset.setSourcePath(srcFile);
            asset.m_settings.generateMips = true;

            auto blob = manager.getOrCompileTexture(asset);
            auto header = TextureHeader{};
            std::memcpy(&header, blob.data(), sizeof(TextureHeader));

            CHECK(header.mipLevels == 1);
        }

        SUBCASE("generateMips=false forces 1 mip level")
        {
            auto srcFile = testDir + "/test.png";
            create2x2PNG(srcFile);

            auto manager = DDCManager{cacheDir};
            auto asset = TextureAsset{};
            asset.setSourcePath(srcFile);
            asset.m_settings.generateMips = false;

            auto blob = manager.getOrCompileTexture(asset);
            auto header = TextureHeader{};
            std::memcpy(&header, blob.data(), sizeof(TextureHeader));

            CHECK(header.mipLevels == 1);
        }

        fs::remove_all(testDir);
        fs::remove_all(cacheDir);
    }

    TEST_CASE("Pixel Data Integrity")
    {
        using namespace april::asset;

        auto const testDir = std::string{"TestAssets_Pixels"};
        auto const cacheDir = std::string{"TestCache_Pixels"};
        auto const srcFile = testDir + "/test.png";

        fs::remove_all(testDir);
        fs::remove_all(cacheDir);
        fs::create_directories(testDir);

        createMinimalPNG(srcFile);

        SUBCASE("Pixel data size matches header.dataSize")
        {
            auto manager = DDCManager{cacheDir};
            auto asset = TextureAsset{};
            asset.setSourcePath(srcFile);

            auto blob = manager.getOrCompileTexture(asset);
            auto header = TextureHeader{};
            std::memcpy(&header, blob.data(), sizeof(TextureHeader));

            auto expectedPixelDataSize = blob.size() - sizeof(TextureHeader);
            CHECK(header.dataSize == expectedPixelDataSize);
        }

        SUBCASE("TexturePayload span points to correct offset")
        {
            auto manager = AssetManager{testDir, cacheDir};

            auto asset = TextureAsset{};
            asset.setSourcePath(srcFile);

            auto blob = std::vector<std::byte>{};
            auto payload = manager.getTextureData(asset, blob);

            // Verify span starts at correct offset
            auto expectedOffset = reinterpret_cast<std::byte const*>(blob.data()) + sizeof(TextureHeader);
            CHECK(payload.pixelData.data() == expectedOffset);
        }

        fs::remove_all(testDir);
        fs::remove_all(cacheDir);
    }
}
