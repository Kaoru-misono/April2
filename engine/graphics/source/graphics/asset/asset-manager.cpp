// #include "asset-manager.hpp"

// namespace april
// {
//     AssetManager::AssetManager(GraphicsContext const& context)
//         : m_context{context}
//     {
//         AP_INFO("AssetManager Initialized.");
//     }

//     AssetManager::~AssetManager()
//     {
//         // RAII handles clearing maps and shared_ptrs.
//         AP_INFO("AssetManager Shutdown.");
//     }

//     auto AssetManager::registerAssetPath(UUID handle, std::filesystem::path const& path) -> void
//     {
//         m_assetRegistry[handle] = path;
//     }
// }
