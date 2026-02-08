#pragma once

#include <editor/tool-window.hpp>
#include <editor/ui/action-manager.hpp>

#include <array>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace april::asset
{
    class AssetManager;
}

namespace april::editor
{
    enum class ContentItemType
    {
        Folder,
        Texture,
        Mesh,
        Material,
        Shader,
        Scene,
        Audio,
        Script,
        Generic
    };

    struct ContentItem
    {
        std::filesystem::path virtualPath{};
        std::filesystem::path physicalPath{};
        std::string displayName{};
        ContentItemType type{ContentItemType::Generic};
        bool isDirectory{false};
    };

    class ContentBrowserWindow final : public ToolWindow
    {
    public:
        ContentBrowserWindow() = default;

        [[nodiscard]] auto title() const -> char const* override { return "Content Browser"; }
        auto onUIRender(EditorContext& context) -> void override;
        auto requestRefresh() -> void;

    private:
        auto ensureInitialized(EditorContext& context) -> void;
        auto registerActions() -> void;
        auto refreshEntries() -> void;
        auto refreshContentItems() -> void;
        auto applyFilters() -> void;

        // UI drawing functions
        auto drawToolbar() -> void;
        auto drawBreadcrumbs() -> void;
        auto drawFolderTree() -> void;
        auto drawFolderTreeNode(std::filesystem::path const& path) -> void;
        auto drawContentGrid(EditorContext& context) -> void;
        auto drawContentItem(EditorContext& context, ContentItem const& item, float itemWidth) -> void;
        auto drawContextMenu(EditorContext& context) -> void;

        // Actions
        auto handleSelection(std::filesystem::path const& path, bool ctrlHeld, bool shiftHeld) -> void;
        auto navigateTo(std::filesystem::path const& path) -> void;
        auto createFolder(EditorContext& context) -> void;
        auto renameItem(EditorContext& context, std::filesystem::path const& path) -> void;
        auto deleteItem(EditorContext& context, std::filesystem::path const& path) -> void;
        auto openInExplorer(std::filesystem::path const& path) -> void;
        auto spawnMeshAsset(EditorContext& context, ContentItem const& item) -> void;

        // Utility
        [[nodiscard]] auto getItemType(std::filesystem::path const& path) const -> ContentItemType;
        [[nodiscard]] static auto getItemIcon(ContentItemType type) -> char const*;
        [[nodiscard]] auto isPathInCurrentDir(std::filesystem::path const& virtualPath) const -> bool;
        [[nodiscard]] auto resolvePhysicalPath(std::filesystem::path const& virtualPath) const -> std::filesystem::path;
        [[nodiscard]] auto toVirtualPath(std::filesystem::path const& physicalPath) const -> std::filesystem::path;

        // Root and navigation
        std::filesystem::path m_rootVirtual{};
        std::filesystem::path m_rootPhysical{};
        std::filesystem::path m_currentVirtual{};
        std::filesystem::path m_currentPhysical{};
        std::vector<std::filesystem::directory_entry> m_entries{};
        bool m_initialized{false};
        bool m_refreshPending{false};
        asset::AssetManager* m_assetManager{};

        // Selection
        std::set<std::filesystem::path> m_selectedPaths{};
        std::filesystem::path m_lastSelectedPath{};

        // Search/Filter
        std::array<char, 256> m_searchBuffer{};
        int m_typeFilter{0}; // 0=All, 1=Folder, 2=Texture, 3=Mesh, etc.

        // UI State
        float m_thumbnailSize{80.0f};
        float m_treePanelWidth{200.0f};
        bool m_showFolderTree{true};

        // Rename state
        bool m_renaming{false};
        std::filesystem::path m_renamingPath{};
        std::array<char, 256> m_renameBuffer{};

        // Delete confirmation
        bool m_showDeleteConfirm{false};
        std::filesystem::path m_deleteTargetPath{};

        // Create folder state
        bool m_showCreateFolderPopup{false};
        std::array<char, 256> m_newFolderName{};

        // Cached items
        std::vector<ContentItem> m_contentItems{};
        std::vector<ContentItem> m_filteredItems{};

        ui::ActionManager m_actionManager{};
        bool m_actionsRegistered{false};

        // Deferred navigation to avoid mutating lists during iteration.
        std::filesystem::path m_pendingNavigation{};
        bool m_hasPendingNavigation{false};
    };
}
