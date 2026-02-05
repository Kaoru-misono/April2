#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <cstring>

#include <core/tools/uuid.hpp>
#include <asset/asset.hpp>
#include <asset/texture-asset.hpp>
#include <asset/static-mesh-asset.hpp>
#include <asset/material-asset.hpp>
#include <asset/blob-header.hpp>
#include <asset/ddc/ddc-key.hpp>
#include <asset/ddc/ddc-utils.hpp>
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

// Helper: Create a minimal valid glTF file (single triangle)
auto createMinimalGLTF(std::string const& path) -> void
{
    static constexpr auto base64Buffer = "AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAABAAIA";

    auto json = std::string{};
    json += "{\n";
    json += "  \"asset\": { \"version\": \"2.0\" },\n";
    json += "  \"buffers\": [\n";
    json += "    {\n";
    json += "      \"uri\": \"data:application/octet-stream;base64,";
    json += base64Buffer;
    json += "\",\n";
    json += "      \"byteLength\": 42\n";
    json += "    }\n";
    json += "  ],\n";
    json += "  \"bufferViews\": [\n";
    json += "    { \"buffer\": 0, \"byteOffset\": 0, \"byteLength\": 36 },\n";
    json += "    { \"buffer\": 0, \"byteOffset\": 36, \"byteLength\": 6 }\n";
    json += "  ],\n";
    json += "  \"accessors\": [\n";
    json += "    {\n";
    json += "      \"bufferView\": 0,\n";
    json += "      \"byteOffset\": 0,\n";
    json += "      \"componentType\": 5126,\n";
    json += "      \"count\": 3,\n";
    json += "      \"type\": \"VEC3\",\n";
    json += "      \"min\": [0, 0, 0],\n";
    json += "      \"max\": [1, 1, 0]\n";
    json += "    },\n";
    json += "    {\n";
    json += "      \"bufferView\": 1,\n";
    json += "      \"byteOffset\": 0,\n";
    json += "      \"componentType\": 5123,\n";
    json += "      \"count\": 3,\n";
    json += "      \"type\": \"SCALAR\"\n";
    json += "    }\n";
    json += "  ],\n";
    json += "  \"meshes\": [\n";
    json += "    {\n";
    json += "      \"primitives\": [\n";
    json += "        {\n";
    json += "          \"attributes\": { \"POSITION\": 0 },\n";
    json += "          \"indices\": 1\n";
    json += "        }\n";
    json += "      ]\n";
    json += "    }\n";
    json += "  ],\n";
    json += "  \"nodes\": [ { \"mesh\": 0 } ],\n";
    json += "  \"scenes\": [ { \"nodes\": [0] } ],\n";
    json += "  \"scene\": 0\n";
    json += "}\n";

    std::ofstream file(path, std::ios::binary);
    file.write(json.data(), static_cast<std::streamsize>(json.size()));
}

auto readBinaryFile(std::string const& path) -> std::vector<std::byte>
{
    auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
    if (!file.is_open())
    {
        return {};
    }

    auto size = static_cast<std::streamsize>(file.tellg());
    auto buffer = std::vector<std::byte>(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
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

        fs::remove_all(testDir);
    }

    TEST_CASE("TextureImporter - Texture Compilation")
    {
        using namespace april::asset;

        auto const testDir = std::string{"TestAssets_DDC"};
        auto const cacheDir = std::string{"TestCache_DDC"};
        auto const srcFile = testDir + "/texture.png";
        auto const assetFile = testDir + "/texture.asset";

        fs::remove_all(testDir);
        fs::remove_all(cacheDir);
        fs::create_directories(testDir);

        createMinimalPNG(srcFile);

        auto writeTextureAsset = [&](std::string const& path, bool sRgb) -> void
        {
            auto asset = TextureAsset{};
            asset.setSourcePath(srcFile);
            asset.m_settings.sRGB = sRgb;
            asset.m_settings.generateMips = false;

            auto json = nlohmann::json{};
            asset.serializeJson(json);

            std::ofstream file(path);
            file << json.dump(2);
        };

        writeTextureAsset(assetFile, false);

        SUBCASE("Compiles valid PNG to binary blob")
        {
            auto manager = AssetManager{testDir, cacheDir};
            auto asset = manager.loadAsset<TextureAsset>(assetFile);

            auto blob = std::vector<std::byte>{};
            auto payload = manager.getTextureData(*asset, blob);

            CHECK(payload.isValid());
            CHECK(payload.header.width == 1);
            CHECK(payload.header.height == 1);
            CHECK(payload.header.channels == 4);
            CHECK(payload.header.format == PixelFormat::RGBA8Unorm);
            CHECK(payload.header.mipLevels == 1);
            CHECK(payload.header.dataSize == 4);
        }

        SUBCASE("sRGB setting affects format")
        {
            auto const linearAssetFile = testDir + "/linear.asset";
            auto const srgbAssetFile = testDir + "/srgb.asset";
            writeTextureAsset(linearAssetFile, false);
            writeTextureAsset(srgbAssetFile, true);

            auto manager = AssetManager{testDir, cacheDir};
            auto assetLinear = manager.loadAsset<TextureAsset>(linearAssetFile);
            auto assetSrgb = manager.loadAsset<TextureAsset>(srgbAssetFile);

            auto blobLinear = std::vector<std::byte>{};
            auto blobSrgb = std::vector<std::byte>{};
            auto payloadLinear = manager.getTextureData(*assetLinear, blobLinear);
            auto payloadSrgb = manager.getTextureData(*assetSrgb, blobSrgb);

            CHECK(payloadLinear.header.format == PixelFormat::RGBA8Unorm);
            CHECK(payloadSrgb.header.format == PixelFormat::RGBA8UnormSrgb);
        }

        SUBCASE("Cache hit returns same data")
        {
            auto manager = AssetManager{testDir, cacheDir};
            auto asset = manager.loadAsset<TextureAsset>(assetFile);

            auto blob1 = std::vector<std::byte>{};
            auto blob2 = std::vector<std::byte>{};
            manager.getTextureData(*asset, blob1);
            manager.getTextureData(*asset, blob2);

            CHECK(blob1 == blob2);
        }

        SUBCASE("Cache entry exists")
        {
            auto manager = AssetManager{testDir, cacheDir};
            auto asset = manager.loadAsset<TextureAsset>(assetFile);

            auto blob = std::vector<std::byte>{};
            manager.getTextureData(*asset, blob);

            auto settingsJson = nlohmann::json{};
            settingsJson["settings"] = asset->m_settings;
            auto key = buildDdcKey(FingerprintInput{
                "TX",
                asset->getHandle().toString(),
                "TextureImporter",
                1,
                "stb_image@unknown|texblob@1",
                hashFileContents(asset->getSourcePath()),
                hashJson(settingsJson),
                hashDependencies({}),
                TargetProfile{}
            });

            CHECK(manager.getDdc().exists(key));
        }

        SUBCASE("Invalid image returns empty payload")
        {
            auto badAssetFile = testDir + "/bad.asset";
            auto asset = TextureAsset{};
            asset.setSourcePath(testDir + "/nonexistent.png");

            {
                auto json = nlohmann::json{};
                asset.serializeJson(json);
                std::ofstream file(badAssetFile);
                file << json.dump(2);
            }

            auto manager = AssetManager{testDir, cacheDir};
            auto loaded = manager.loadAsset<TextureAsset>(badAssetFile);
            REQUIRE(loaded != nullptr);

            auto blob = std::vector<std::byte>{};
            auto payload = manager.getTextureData(*loaded, blob);

            CHECK(!payload.isValid());
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

            auto assetFile = testDir + "/1x1.asset";
            auto asset = TextureAsset{};
            asset.setSourcePath(srcFile);
            asset.m_settings.generateMips = true;

            {
                auto json = nlohmann::json{};
                asset.serializeJson(json);
                std::ofstream file(assetFile);
                file << json.dump(2);
            }

            auto manager = AssetManager{testDir, cacheDir};
            auto loaded = manager.loadAsset<TextureAsset>(assetFile);
            REQUIRE(loaded != nullptr);

            auto blob = std::vector<std::byte>{};
            auto payload = manager.getTextureData(*loaded, blob);

            CHECK(payload.header.mipLevels == 1);
        }

        SUBCASE("generateMips=false forces 1 mip level")
        {
            auto srcFile = testDir + "/test.png";
            create2x2PNG(srcFile);

            auto assetFile = testDir + "/test.asset";
            auto asset = TextureAsset{};
            asset.setSourcePath(srcFile);
            asset.m_settings.generateMips = false;

            {
                auto json = nlohmann::json{};
                asset.serializeJson(json);
                std::ofstream file(assetFile);
                file << json.dump(2);
            }

            auto manager = AssetManager{testDir, cacheDir};
            auto loaded = manager.loadAsset<TextureAsset>(assetFile);
            REQUIRE(loaded != nullptr);

            auto blob = std::vector<std::byte>{};
            auto payload = manager.getTextureData(*loaded, blob);

            CHECK(payload.header.mipLevels == 1);
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
            auto assetFile = testDir + "/pixels.asset";
            auto asset = TextureAsset{};
            asset.setSourcePath(srcFile);

            {
                auto json = nlohmann::json{};
                asset.serializeJson(json);
                std::ofstream file(assetFile);
                file << json.dump(2);
            }

            auto manager = AssetManager{testDir, cacheDir};
            auto loaded = manager.loadAsset<TextureAsset>(assetFile);
            REQUIRE(loaded != nullptr);

            auto blob = std::vector<std::byte>{};
            auto payload = manager.getTextureData(*loaded, blob);

            auto expectedPixelDataSize = blob.size() - sizeof(TextureHeader);
            CHECK(payload.header.dataSize == expectedPixelDataSize);
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

    TEST_CASE("MeshHeader - Binary Layout")
    {
        using namespace april::asset;

        SUBCASE("Header size is fixed at 80 bytes")
        {
            CHECK(sizeof(MeshHeader) == 80);
        }

        SUBCASE("Header magic value is correct")
        {
            auto header = MeshHeader{};
            CHECK(header.magic == 0x41504D58); // "APMX"
        }

        SUBCASE("Default header is invalid (zero counts)")
        {
            auto header = MeshHeader{};
            CHECK_FALSE(header.isValid());
        }

        SUBCASE("Header with valid counts is valid")
        {
            auto header = MeshHeader{};
            header.vertexCount = 3;
            header.indexCount = 3;
            CHECK(header.isValid());
        }

        SUBCASE("Header with wrong magic is invalid")
        {
            auto header = MeshHeader{};
            header.vertexCount = 3;
            header.indexCount = 3;
            header.magic = 0x12345678;
            CHECK_FALSE(header.isValid());
        }
    }

    TEST_CASE("MeshPayload - Validation")
    {
        using namespace april::asset;

        SUBCASE("Empty payload is invalid")
        {
            auto payload = MeshPayload{};
            CHECK_FALSE(payload.isValid());
        }

        SUBCASE("Payload with valid header but empty data is invalid")
        {
            auto payload = MeshPayload{};
            payload.header.vertexCount = 3;
            payload.header.indexCount = 3;
            CHECK_FALSE(payload.isValid());
        }

        SUBCASE("Payload with valid header and data is valid")
        {
            auto vertexData = std::vector<std::byte>(144); // 3 vertices * 48 bytes
            auto indexData = std::vector<std::byte>(12);   // 3 indices * 4 bytes
            auto submeshes = std::vector<Submesh>(1);

            auto payload = MeshPayload{};
            payload.header.vertexCount = 3;
            payload.header.indexCount = 3;
            payload.submeshes = std::span<Submesh const>{submeshes};
            payload.vertexData = std::span<std::byte const>{vertexData};
            payload.indexData = std::span<std::byte const>{indexData};

            CHECK(payload.isValid());
        }
    }

    TEST_CASE("StaticMeshAsset - JSON Serialization")
    {
        using namespace april;
        using namespace april::asset;

        auto const testDir = std::string{"TestAssets_MeshSerialize"};
        auto const srcFile = testDir + "/test.gltf";

        fs::create_directories(testDir);

        SUBCASE("Serialize and deserialize preserves data")
        {
            auto asset = StaticMeshAsset{};
            asset.setSourcePath(srcFile);
            asset.m_settings.optimize = true;
            asset.m_settings.generateTangents = true;
            asset.m_settings.flipWindingOrder = false;
            asset.m_settings.scale = 2.0f;
            auto matGuid0 = core::UUID{"00000000-0000-0000-0000-000000000001"};
            auto matGuid1 = core::UUID{"00000000-0000-0000-0000-000000000002"};
            asset.m_materialSlots = {
                MaterialSlot{"Default", AssetRef{matGuid0, 0}},
                MaterialSlot{"Detail", AssetRef{matGuid1, 0}}
            };

            auto json = nlohmann::json{};
            asset.serializeJson(json);

            CHECK(json["source_path"] == srcFile);
            CHECK(json["type"] == "Mesh");
            CHECK(json["settings"]["optimize"] == true);
            CHECK(json["settings"]["generateTangents"] == true);
            CHECK(json["settings"]["flipWindingOrder"] == false);
            CHECK(json["settings"]["scale"] == doctest::Approx(2.0f));
            CHECK(json["materialSlots"].size() == 2);
            CHECK(json["materialSlots"][0]["name"] == "Default");
            CHECK(json["materialSlots"][0]["materialRef"]["guid"] == matGuid0.toString());

            auto asset2 = StaticMeshAsset{};
            CHECK(asset2.deserializeJson(json));
            CHECK(asset2.getSourcePath() == srcFile);
            CHECK(asset2.m_settings.optimize == true);
            CHECK(asset2.m_settings.generateTangents == true);
            CHECK(asset2.m_settings.flipWindingOrder == false);
            CHECK(asset2.m_settings.scale == doctest::Approx(2.0f));
            REQUIRE(asset2.m_materialSlots.size() == 2);
            CHECK(asset2.m_materialSlots[0].name == "Default");
            CHECK(asset2.m_materialSlots[0].materialRef.guid == matGuid0);
            REQUIRE(asset2.getReferences().size() == 2);
            CHECK(asset2.getReferences()[0].guid == matGuid0);
            CHECK(asset2.getReferences()[1].guid == matGuid1);
        }

        fs::remove_all(testDir);
    }

    TEST_CASE("MeshImporter - Mesh Compilation")
    {
        using namespace april::asset;

        auto const testDir = std::string{"TestAssets_MeshDDC"};
        auto const cacheDir = std::string{"TestCache_MeshDDC"};
        auto const srcFile = testDir + "/triangle.gltf";
        auto const assetFile = testDir + "/triangle.gltf.asset";

        fs::remove_all(testDir);
        fs::remove_all(cacheDir);
        fs::create_directories(testDir);

        createMinimalGLTF(srcFile);

        {
            auto asset = StaticMeshAsset{};
            asset.setSourcePath(srcFile);
            auto json = nlohmann::json{};
            asset.serializeJson(json);
            std::ofstream file(assetFile);
            file << json.dump(2);
        }

        SUBCASE("Compiles valid glTF to binary blob")
        {
            auto manager = AssetManager{testDir, cacheDir};
            auto asset = manager.loadAsset<StaticMeshAsset>(assetFile);

            auto blob = std::vector<std::byte>{};
            auto payload = manager.getMeshData(*asset, blob);

            CHECK(payload.isValid());
            CHECK(payload.header.vertexCount == 3);
            CHECK(payload.header.indexCount == 3);
            CHECK(payload.header.submeshCount == 1);
            CHECK(payload.header.vertexStride == 48);
            CHECK(payload.header.indexFormat == 1);
            CHECK(payload.header.vertexDataSize == 144);
            CHECK(payload.header.indexDataSize == 12);
            CHECK(payload.header.vertexDataSize == static_cast<uint64_t>(payload.header.vertexCount) * payload.header.vertexStride);

            auto expectedSize = sizeof(MeshHeader) + sizeof(Submesh) + payload.header.vertexDataSize + payload.header.indexDataSize;
            CHECK(blob.size() == expectedSize);
        }

        SUBCASE("Cache hit returns same data")
        {
            auto manager = AssetManager{testDir, cacheDir};
            auto asset = manager.loadAsset<StaticMeshAsset>(assetFile);

            auto blob1 = std::vector<std::byte>{};
            auto blob2 = std::vector<std::byte>{};
            manager.getMeshData(*asset, blob1);
            manager.getMeshData(*asset, blob2);

            CHECK(blob1 == blob2);
        }

        SUBCASE("Cache entry exists")
        {
            auto manager = AssetManager{testDir, cacheDir};
            auto asset = manager.loadAsset<StaticMeshAsset>(assetFile);

            auto blob = std::vector<std::byte>{};
            manager.getMeshData(*asset, blob);

            auto settingsJson = nlohmann::json{};
            settingsJson["settings"] = asset->m_settings;
            auto key = buildDdcKey(FingerprintInput{
                "MS",
                asset->getHandle().toString(),
                "MeshImporter",
                1,
                "tinygltf@unknown|meshopt@unknown|meshblob@1",
                hashFileContents(asset->getSourcePath()),
                hashJson(settingsJson),
                hashDependencies({}),
                TargetProfile{}
            });

            CHECK(manager.getDdc().exists(key));
        }

        fs::remove_all(testDir);
        fs::remove_all(cacheDir);
    }

    TEST_CASE("AssetManager - Mesh Loading")
    {
        using namespace april::asset;

        auto const testDir = std::string{"TestAssets_MeshManager"};
        auto const cacheDir = std::string{"TestCache_MeshManager"};
        auto const srcFile = testDir + "/triangle.gltf";
        auto const assetFile = testDir + "/triangle.gltf.asset";

        fs::remove_all(testDir);
        fs::remove_all(cacheDir);
        fs::create_directories(testDir);

        createMinimalGLTF(srcFile);

        {
            auto asset = StaticMeshAsset{};
            asset.setSourcePath(srcFile);
            auto json = nlohmann::json{};
            asset.serializeJson(json);
            std::ofstream file(assetFile);
            file << json.dump(2);
        }

        SUBCASE("Get mesh data returns valid payload")
        {
            auto manager = AssetManager{testDir, cacheDir};
            auto asset = manager.loadAsset<StaticMeshAsset>(assetFile);
            REQUIRE(asset != nullptr);

            auto blob = std::vector<std::byte>{};
            auto payload = manager.getMeshData(*asset, blob);

            CHECK(payload.isValid());
            CHECK(payload.header.vertexCount == 3);
            CHECK(payload.header.indexCount == 3);
            CHECK(payload.submeshes.size() == 1);
            CHECK(payload.vertexData.size() == payload.header.vertexDataSize);
            CHECK(payload.indexData.size() == payload.header.indexDataSize);
            CHECK(payload.submeshes[0].indexOffset == 0);
            CHECK(payload.submeshes[0].indexCount == 3);
        }

        fs::remove_all(testDir);
        fs::remove_all(cacheDir);
    }

    TEST_CASE("Submesh Structure")
    {
        using namespace april::asset;

        SUBCASE("Submesh has correct layout")
        {
            auto submesh = Submesh{};
            submesh.indexOffset = 0;
            submesh.indexCount = 36;
            submesh.materialIndex = 1;

            CHECK(submesh.indexOffset == 0);
            CHECK(submesh.indexCount == 36);
            CHECK(submesh.materialIndex == 1);
        }
    }

    TEST_CASE("MaterialAsset - JSON Serialization")
    {
        using namespace april::asset;

        SUBCASE("Default material parameters")
        {
            auto material = MaterialAsset{};

            CHECK(material.parameters.baseColorFactor.x == doctest::Approx(1.0f));
            CHECK(material.parameters.baseColorFactor.y == doctest::Approx(1.0f));
            CHECK(material.parameters.baseColorFactor.z == doctest::Approx(1.0f));
            CHECK(material.parameters.baseColorFactor.w == doctest::Approx(1.0f));
            CHECK(material.parameters.metallicFactor == doctest::Approx(1.0f));
            CHECK(material.parameters.roughnessFactor == doctest::Approx(1.0f));
            CHECK(material.parameters.alphaMode == "OPAQUE");
            CHECK(material.parameters.doubleSided == false);
        }

        SUBCASE("Serialize and deserialize preserves data")
        {
            auto material = MaterialAsset{};
            material.parameters.baseColorFactor = {0.8f, 0.2f, 0.2f, 1.0f};
            material.parameters.metallicFactor = 0.0f;
            material.parameters.roughnessFactor = 0.5f;
            material.parameters.emissiveFactor = {0.1f, 0.0f, 0.0f};
            material.parameters.alphaMode = "MASK";
            material.parameters.alphaCutoff = 0.3f;
            material.parameters.doubleSided = true;

            auto json = nlohmann::json{};
            material.serializeJson(json);

            CHECK(json["type"] == "Material");
            CHECK(json["parameters"]["metallicFactor"] == doctest::Approx(0.0f));
            CHECK(json["parameters"]["roughnessFactor"] == doctest::Approx(0.5f));
            CHECK(json["parameters"]["alphaMode"] == "MASK");
            CHECK(json["parameters"]["alphaCutoff"] == doctest::Approx(0.3f));
            CHECK(json["parameters"]["doubleSided"] == true);

            auto material2 = MaterialAsset{};
            CHECK(material2.deserializeJson(json));
            CHECK(material2.parameters.baseColorFactor.x == doctest::Approx(0.8f));
            CHECK(material2.parameters.baseColorFactor.y == doctest::Approx(0.2f));
            CHECK(material2.parameters.baseColorFactor.z == doctest::Approx(0.2f));
            CHECK(material2.parameters.metallicFactor == doctest::Approx(0.0f));
            CHECK(material2.parameters.roughnessFactor == doctest::Approx(0.5f));
            CHECK(material2.parameters.emissiveFactor.x == doctest::Approx(0.1f));
            CHECK(material2.parameters.alphaMode == "MASK");
            CHECK(material2.parameters.alphaCutoff == doctest::Approx(0.3f));
            CHECK(material2.parameters.doubleSided == true);
        }

        SUBCASE("Texture references are serialized")
        {
            using namespace april;

            auto material = MaterialAsset{};
            auto baseGuid = core::UUID{"00000000-0000-0000-0000-000000000010"};
            auto normalGuid = core::UUID{"00000000-0000-0000-0000-000000000011"};
            material.textures.baseColorTexture = TextureReference{AssetRef{baseGuid, 0}, 0};
            material.textures.normalTexture = TextureReference{AssetRef{normalGuid, 0}, 1};

            auto json = nlohmann::json{};
            material.serializeJson(json);

            CHECK(json["textures"].contains("baseColorTexture"));
            CHECK(json["textures"]["baseColorTexture"]["asset"]["guid"] == baseGuid.toString());
            CHECK(json["textures"]["baseColorTexture"]["texCoord"] == 0);
            CHECK(json["textures"].contains("normalTexture"));
            CHECK(json["textures"]["normalTexture"]["asset"]["guid"] == normalGuid.toString());
            CHECK(json["textures"]["normalTexture"]["texCoord"] == 1);

            auto material2 = MaterialAsset{};
            CHECK(material2.deserializeJson(json));
            REQUIRE(material2.textures.baseColorTexture.has_value());
            CHECK(material2.textures.baseColorTexture->asset.guid == baseGuid);
            CHECK(material2.textures.baseColorTexture->texCoord == 0);
            REQUIRE(material2.textures.normalTexture.has_value());
            CHECK(material2.textures.normalTexture->asset.guid == normalGuid);
            CHECK(material2.textures.normalTexture->texCoord == 1);
            REQUIRE(material2.getReferences().size() == 2);
            CHECK(material2.getReferences()[0].guid == baseGuid);
            CHECK(material2.getReferences()[1].guid == normalGuid);
        }

    }

    TEST_CASE("AssetManager - Material Loading")
    {
        using namespace april::asset;

        auto const testDir = std::string{"TestAssets_Material"};
        auto const materialFile = testDir + "/red_metal.material.asset";

        fs::remove_all(testDir);
        fs::create_directories(testDir);

        // Create material asset file
        {
            auto material = MaterialAsset{};
            material.parameters.baseColorFactor = {0.8f, 0.2f, 0.2f, 1.0f};
            material.parameters.metallicFactor = 1.0f;
            material.parameters.roughnessFactor = 0.3f;

            auto json = nlohmann::json{};
            material.serializeJson(json);

            std::ofstream file(materialFile);
            file << json.dump(2);
        }

        SUBCASE("Load material from file")
        {
            auto manager = AssetManager{testDir, "TestCache_Material"};
            auto material = manager.loadAsset<MaterialAsset>(materialFile);

            REQUIRE(material != nullptr);
            CHECK(material->getType() == AssetType::Material);
            CHECK(material->parameters.baseColorFactor.x == doctest::Approx(0.8f));
            CHECK(material->parameters.baseColorFactor.y == doctest::Approx(0.2f));
            CHECK(material->parameters.metallicFactor == doctest::Approx(1.0f));
            CHECK(material->parameters.roughnessFactor == doctest::Approx(0.3f));
        }

        SUBCASE("Save and load material roundtrip")
        {
            using namespace april;

            auto const saveFile = testDir + "/test_save.material.asset";

            auto originalMaterial = std::make_shared<MaterialAsset>();
            originalMaterial->parameters.baseColorFactor = {0.5f, 0.5f, 0.9f, 1.0f};
            originalMaterial->parameters.metallicFactor = 0.2f;
            originalMaterial->parameters.roughnessFactor = 0.8f;
            auto baseGuid = core::UUID{"00000000-0000-0000-0000-000000000020"};
            originalMaterial->textures.baseColorTexture = TextureReference{AssetRef{baseGuid, 0}, 0};

            auto manager = AssetManager{testDir, "TestCache_Material"};
            CHECK(manager.saveMaterialAsset(originalMaterial, saveFile));
            CHECK(fs::exists(saveFile));

            auto loadedMaterial = manager.loadAsset<MaterialAsset>(saveFile);
            REQUIRE(loadedMaterial != nullptr);
            CHECK(loadedMaterial->parameters.baseColorFactor.x == doctest::Approx(0.5f));
            CHECK(loadedMaterial->parameters.baseColorFactor.y == doctest::Approx(0.5f));
            CHECK(loadedMaterial->parameters.baseColorFactor.z == doctest::Approx(0.9f));
            CHECK(loadedMaterial->parameters.metallicFactor == doctest::Approx(0.2f));
            CHECK(loadedMaterial->parameters.roughnessFactor == doctest::Approx(0.8f));
            REQUIRE(loadedMaterial->textures.baseColorTexture.has_value());
            CHECK(loadedMaterial->textures.baseColorTexture->asset.guid == baseGuid);
        }

        fs::remove_all(testDir);
        fs::remove_all("TestCache_Material");
    }
}
