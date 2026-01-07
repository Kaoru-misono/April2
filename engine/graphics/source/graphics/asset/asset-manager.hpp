// #pragma once

// #include "asset.hpp"

// #include <core/log/logger.hpp>
// #include <unordered_map>
// #include <filesystem>

// namespace april
// {
//     struct AssetManager
//     {
//         // Pass the graphics context so we can load textures/buffers.
//         AssetManager(GraphicsContext const& context);
//         ~AssetManager();

//         // 1. Get Asset by UUID (Primary method for Runtime).
//         // Checks cache first, then tries to load from disk using the registry.
//         template <typename T>
//         [[nodiscard]] auto getAsset(UUID handle) -> std::shared_ptr<T>
//         {
//             // Check memory cache.
//             if (m_loadedAssets.contains(handle))
//             {
//                 return std::static_pointer_cast<T>(m_loadedAssets.at(handle));
//             }

//             // Not in memory? Check Registry to find file path.
//             if (m_assetRegistry.contains(handle))
//             {
//                 auto path = m_assetRegistry.at(handle);
//                 return loadAssetFromFile<T>(path);
//             }

//             AP_ERROR("Asset UUID not found in registry: {}", handle.toString());
//             return nullptr;
//         }

//         // 2. Import Raw File (Editor/Tooling).
//         // Imports a source file (e.g. .png), converts to .asset, and caches it.
//         template <typename T>
//         auto import(std::string const& rawPath, std::string const& assetPath = {}) -> std::shared_ptr<T>
//         {
//             auto newAsset = std::shared_ptr<T>{};

//             // Dispatch based on type.
//             // if constexpr (std::is_same_v<T, TextureAsset>) {
//             //     // Generate default .asset path if not provided (e.g., "texture.png.asset").
//             //     auto targetPath = assetPath.empty() ? (rawPath + ".asset") : assetPath;
//             //     // TODO: Directly load from asset path after check.
//             //     newAsset = TextureAsset::import(m_context, rawPath, targetPath);
//             // }
//             // else if constexpr (std::is_same_v<T, Mesh>) ...

//             if (newAsset)
//             {
//                 // Register and cache.
//                 auto handle = newAsset->getHandle();
//                 m_loadedAssets[handle] = newAsset;
//                 m_assetRegistry[handle] = newAsset->getAssetPath(); // Store path for future reloading.

//                 return newAsset;
//             }

//             return nullptr;
//         }

//         // Manually register a known asset file (e.g., scanning disk at startup).
//         auto registerAssetPath(UUID handle, std::filesystem::path const& path) -> void;

//     private:
//         // Load the .asset metadata file, then load the resource.
//         template <typename T>
//         auto loadAssetFromFile(std::filesystem::path const& assetPath) -> std::shared_ptr<T>
//         {
//             auto asset = std::shared_ptr<T>{};

//             // if constexpr (std::is_same_v<T, TextureAsset>) {
//             //     asset = TextureAsset::loadFromAssetFile(m_context, assetPath.string());
//             // }

//             if (asset)
//             {
//                 m_loadedAssets[asset->getHandle()] = asset;
//                 return asset;
//             }

//             return nullptr;
//         }

//     private:
//         // GraphicsContext const& m_context{};

//         // Cache: Assets currently in memory.
//         std::unordered_map<UUID, std::shared_ptr<Asset>> m_loadedAssets{};

//         // Registry: Maps UUID -> Path to .asset file on disk.
//         // This acts as a database of all available assets in the project.
//         std::unordered_map<UUID, std::filesystem::path> m_assetRegistry{};
//     };
// }
