#pragma once

#include "../texture-asset.hpp"
#include "../static-mesh-asset.hpp"

#include <vector>
#include <filesystem>

namespace april::asset
{
    /**
     * DDCManager - Derived Data Cache Manager.
     * Handles caching and compilation of texture and mesh assets into binary blobs.
     */
    class DDCManager
    {
    public:
        explicit DDCManager(std::filesystem::path cacheRoot = "Cache/DDC");

        /**
         * Get or compile a texture asset.
         * Returns the compiled binary blob (header + pixel data).
         * Uses cache if available, otherwise compiles from source.
         */
        auto getOrCompileTexture(TextureAsset const& asset) -> std::vector<std::byte>;

        /**
         * Get or compile a mesh asset.
         * Returns the compiled binary blob (header + submeshes + vertex data + index data).
         * Uses cache if available, otherwise compiles from source.
         */
        auto getOrCompileMesh(StaticMeshAsset const& asset) -> std::vector<std::byte>;

    private:
        std::filesystem::path m_cacheRoot;

        auto compileInternal(TextureAsset const& asset) -> std::vector<std::byte>;
        auto compileInternalMesh(StaticMeshAsset const& asset) -> std::vector<std::byte>;
        auto loadFile(std::filesystem::path const& path) -> std::vector<std::byte>;
        auto saveFile(std::filesystem::path const& path, std::vector<std::byte> const& data) -> void;

        static auto calculateMipLevels(int width, int height) -> uint32_t;
    };

} // namespace april::asset
