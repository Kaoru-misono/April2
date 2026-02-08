#include "content-browser-window.hpp"

#include <editor/editor-context.hpp>
#include <editor/ui/ui.hpp>
#include <asset/asset-manager.hpp>
#include <core/file/vfs.hpp>
#include <runtime/engine.hpp>
#include <scene/scene.hpp>
#include <imgui.h>

#include "font/fonts.hpp"
#include "font/external/IconsMaterialSymbols.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <format>

namespace april::editor
{
    namespace
    {
        auto toLowerString(std::string_view str) -> std::string
        {
            auto result = std::string{str};
            std::transform(result.begin(), result.end(), result.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return result;
        }

        auto containsIgnoreCase(std::string_view haystack, std::string_view needle) -> bool
        {
            if (needle.empty())
            {
                return true;
            }
            auto lowerHaystack = toLowerString(haystack);
            auto lowerNeedle = toLowerString(needle);
            return lowerHaystack.find(lowerNeedle) != std::string::npos;
        }

        auto getLowerExtension(std::filesystem::path const& path) -> std::string
        {
            auto ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return ext;
        }

        auto buildDisplayName(std::filesystem::path const& path, bool isDirectory) -> std::string
        {
            if (isDirectory)
            {
                return path.filename().string();
            }

            auto name = path.filename();
            while (name.has_extension())
            {
                name = name.stem();
            }
            return name.string();
        }

        auto getHiddenSuffix(std::filesystem::path const& path, bool isDirectory) -> std::string
        {
            if (isDirectory)
            {
                return {};
            }

            auto const fileName = path.filename().string();
            auto const displayName = buildDisplayName(path, false);
            if (displayName.size() >= fileName.size())
            {
                return {};
            }

            return fileName.substr(displayName.size());
        }

        constexpr auto kTypeFilterNames = std::array{
            "All",
            "Folders",
            "Textures",
            "Meshes",
            "Materials",
            "Shaders",
            "Scenes",
            "Audio",
            "Scripts"
        };
    }

    auto ContentBrowserWindow::ensureInitialized(EditorContext& context) -> void
    {
        if (m_initialized)
        {
            return;
        }

        m_assetManager = context.assetManager;

        if (context.assetManager)
        {
            m_rootVirtual = context.assetManager->getAssetRoot();
        }
        else
        {
            m_rootVirtual = "content";
        }

        m_rootPhysical = resolvePhysicalPath(m_rootVirtual);
        m_currentVirtual = m_rootVirtual;
        m_currentPhysical = m_rootPhysical;
        refreshContentItems();
        registerActions();
        m_initialized = true;
    }

    auto ContentBrowserWindow::registerActions() -> void
    {
        if (m_actionsRegistered)
        {
            return;
        }
        m_actionsRegistered = true;

        m_actionManager.registerAction(
            "",
            "Rename",
            "F2",
            [this]() {
                if (m_selectedPaths.size() != 1)
                {
                    return;
                }
                auto path = *m_selectedPaths.begin();
                m_renaming = true;
                m_renamingPath = path;
                auto const physicalPath = resolvePhysicalPath(path);
                auto const isDirectory = std::filesystem::is_directory(physicalPath);
                auto name = buildDisplayName(path, isDirectory);
                std::copy(name.begin(), name.end(), m_renameBuffer.begin());
                m_renameBuffer[name.size()] = '\0';
            },
            [this]() { return !m_renaming && m_selectedPaths.size() == 1; }
        );

        m_actionManager.registerAction(
            "",
            "Delete",
            "Delete",
            [this]() {
                if (m_selectedPaths.empty())
                {
                    return;
                }
                m_showDeleteConfirm = true;
                m_deleteTargetPath = *m_selectedPaths.begin();
            },
            [this]() { return !m_renaming && !m_selectedPaths.empty(); }
        );

        m_actionManager.registerAction(
            "",
            "NavigateUp",
            "Backspace",
            [this]() {
                if (m_currentVirtual != m_rootVirtual)
                {
                    navigateTo(m_currentVirtual.parent_path());
                }
            },
            [this]() { return !m_renaming && m_currentVirtual != m_rootVirtual; }
        );

        m_actionManager.registerAction(
            "",
            "CancelRename",
            "Escape",
            [this]() {
                m_renaming = false;
                m_renamingPath.clear();
            },
            [this]() { return m_renaming; }
        );
    }

    auto ContentBrowserWindow::requestRefresh() -> void
    {
        m_refreshPending = true;
    }

    auto ContentBrowserWindow::refreshEntries() -> void
    {
        m_entries.clear();

        if (m_currentPhysical.empty() || !std::filesystem::exists(m_currentPhysical))
        {
            return;
        }

        for (auto const& entry : std::filesystem::directory_iterator{m_currentPhysical})
        {
            m_entries.emplace_back(entry);
        }

        std::sort(m_entries.begin(), m_entries.end(), [](auto const& a, auto const& b) {
            if (a.is_directory() != b.is_directory())
            {
                return a.is_directory();
            }
            return a.path().filename().string() < b.path().filename().string();
        });
    }

    auto ContentBrowserWindow::refreshContentItems() -> void
    {
        refreshEntries();
        m_contentItems.clear();

        for (auto const& entry : m_entries)
        {
            auto const isDirectory = entry.is_directory();
            if (!isDirectory && getLowerExtension(entry.path()) != ".asset")
            {
                continue;
            }

            auto item = ContentItem{};
            item.physicalPath = entry.path();
            item.virtualPath = toVirtualPath(entry.path());
            item.displayName = buildDisplayName(entry.path(), isDirectory);
            item.isDirectory = isDirectory;
            item.type = item.isDirectory ? ContentItemType::Folder : getItemType(entry.path());
            m_contentItems.push_back(std::move(item));
        }

        applyFilters();
    }

    auto ContentBrowserWindow::applyFilters() -> void
    {
        m_filteredItems.clear();

        auto searchQuery = std::string_view{m_searchBuffer.data()};

        for (auto const& item : m_contentItems)
        {
            // Apply search filter
            if (!containsIgnoreCase(item.displayName, searchQuery))
            {
                continue;
            }

            // Apply type filter
            if (m_typeFilter != 0)
            {
                auto expectedType = static_cast<ContentItemType>(m_typeFilter - 1);
                if (item.type != expectedType)
                {
                    continue;
                }
            }

            m_filteredItems.push_back(item);
        }
    }

    auto ContentBrowserWindow::getItemType(std::filesystem::path const& path) const -> ContentItemType
    {
        auto ext = getLowerExtension(path);

        if (ext == ".asset" && m_assetManager)
        {
            auto asset = m_assetManager->loadAssetMetadata(path);
            if (asset)
            {
                switch (asset->getType())
                {
                    case asset::AssetType::Texture: return ContentItemType::Texture;
                    case asset::AssetType::Mesh: return ContentItemType::Mesh;
                    case asset::AssetType::Material: return ContentItemType::Material;
                    case asset::AssetType::Shader: return ContentItemType::Shader;
                    default: break;
                }
            }
        }

        // Textures
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
            ext == ".tga" || ext == ".dds" || ext == ".hdr" || ext == ".exr")
        {
            return ContentItemType::Texture;
        }

        // Meshes
        if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" ||
            ext == ".dae" || ext == ".ply" || ext == ".stl")
        {
            return ContentItemType::Mesh;
        }

        // Materials
        if (ext == ".mat" || ext == ".material")
        {
            return ContentItemType::Material;
        }

        // Shaders
        if (ext == ".slang" || ext == ".hlsl" || ext == ".glsl" || ext == ".vert" ||
            ext == ".frag" || ext == ".comp" || ext == ".geom" || ext == ".shader")
        {
            return ContentItemType::Shader;
        }

        // Scenes
        if (ext == ".scene" || ext == ".world" || ext == ".level")
        {
            return ContentItemType::Scene;
        }

        // Audio
        if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac")
        {
            return ContentItemType::Audio;
        }

        // Scripts
        if (ext == ".lua" || ext == ".py" || ext == ".cs" || ext == ".cpp" || ext == ".h" || ext == ".hpp")
        {
            return ContentItemType::Script;
        }

        return ContentItemType::Generic;
    }

    auto ContentBrowserWindow::getItemIcon(ContentItemType type) -> char const*
    {
        switch (type)
        {
            case ContentItemType::Folder:   return ICON_MS_FOLDER;
            case ContentItemType::Texture:  return ICON_MS_IMAGE;
            case ContentItemType::Mesh:     return ICON_MS_VIEW_IN_AR;
            case ContentItemType::Material: return ICON_MS_TEXTURE;
            case ContentItemType::Shader:   return ICON_MS_CODE;
            case ContentItemType::Scene:    return ICON_MS_SCENE;
            case ContentItemType::Audio:    return ICON_MS_AUDIO_FILE;
            case ContentItemType::Script:   return ICON_MS_DESCRIPTION;
            case ContentItemType::Generic:
            default:                        return ICON_MS_DESCRIPTION;
        }
    }

    auto ContentBrowserWindow::isPathInCurrentDir(std::filesystem::path const& virtualPath) const -> bool
    {
        return virtualPath.parent_path() == m_currentVirtual;
    }

    auto ContentBrowserWindow::resolvePhysicalPath(std::filesystem::path const& virtualPath) const -> std::filesystem::path
    {
        return VFS::resolvePath(virtualPath.generic_string());
    }

    auto ContentBrowserWindow::toVirtualPath(std::filesystem::path const& physicalPath) const -> std::filesystem::path
    {
        auto relEc = std::error_code{};
        auto relPath = std::filesystem::relative(physicalPath, m_rootPhysical, relEc);
        if (relEc)
        {
            return m_rootVirtual;
        }

        if (relPath.empty() || relPath == ".")
        {
            return m_rootVirtual;
        }

        return m_rootVirtual / relPath;
    }

    auto ContentBrowserWindow::navigateTo(std::filesystem::path const& path) -> void
    {
        auto physicalPath = resolvePhysicalPath(path);
        if (std::filesystem::exists(physicalPath) && std::filesystem::is_directory(physicalPath))
        {
            m_currentVirtual = path;
            m_currentPhysical = physicalPath;
            m_selectedPaths.clear();
            m_lastSelectedPath.clear();
            refreshContentItems();
        }
    }

    auto ContentBrowserWindow::handleSelection(std::filesystem::path const& path, bool ctrlHeld, bool shiftHeld) -> void
    {
        if (shiftHeld && !m_lastSelectedPath.empty())
        {
            // Range select
            auto startIt = std::find_if(m_filteredItems.begin(), m_filteredItems.end(),
                [this](auto const& item) { return item.virtualPath == m_lastSelectedPath; });
            auto endIt = std::find_if(m_filteredItems.begin(), m_filteredItems.end(),
                [&path](auto const& item) { return item.virtualPath == path; });

            if (startIt != m_filteredItems.end() && endIt != m_filteredItems.end())
            {
                if (startIt > endIt)
                {
                    std::swap(startIt, endIt);
                }
                if (!ctrlHeld)
                {
                    m_selectedPaths.clear();
                }
                for (auto it = startIt; it <= endIt; ++it)
                {
                    m_selectedPaths.insert(it->virtualPath);
                }
            }
        }
        else if (ctrlHeld)
        {
            // Toggle selection
            if (m_selectedPaths.contains(path))
            {
                m_selectedPaths.erase(path);
            }
            else
            {
                m_selectedPaths.insert(path);
            }
        }
        else
        {
            // Single select
            m_selectedPaths.clear();
            m_selectedPaths.insert(path);
        }

        m_lastSelectedPath = path;
    }

    auto ContentBrowserWindow::drawToolbar() -> void
    {
        auto toolbar = ui::Toolbar{};

        if (toolbar.inputTextWithHint("##Search", ICON_MS_SEARCH " Search...", m_searchBuffer.data(), m_searchBuffer.size(), 200.0f))
        {
            applyFilters();
        }

        if (toolbar.combo("##TypeFilter", &m_typeFilter, kTypeFilterNames.data(), static_cast<int>(kTypeFilterNames.size()), 100.0f))
        {
            applyFilters();
        }

        toolbar.sliderFloat("##Size", &m_thumbnailSize, 40.0f, 150.0f, "%.0f", 100.0f);

        toolbar.toggleButton(
            ICON_MS_CHEVRON_LEFT,
            ICON_MS_CHEVRON_RIGHT,
            &m_showFolderTree,
            "Hide folder tree",
            "Show folder tree"
        );

        if (toolbar.button(ICON_MS_REFRESH, "Refresh"))
        {
            refreshContentItems();
        }
    }

    auto ContentBrowserWindow::drawBreadcrumbs() -> void
    {
        // Home button
        if (ImGui::SmallButton(ICON_MS_HOME))
        {
            navigateTo(m_rootVirtual);
        }

        auto relativePath = m_currentVirtual.lexically_relative(m_rootVirtual);
        if (!relativePath.empty() && relativePath != ".")
        {
            auto accumulated = m_rootVirtual;
            for (auto const& segment : relativePath)
            {
                ImGui::SameLine();
                ImGui::TextUnformatted(ICON_MS_CHEVRON_RIGHT);
                ImGui::SameLine();

                accumulated /= segment;
                auto segmentName = segment.string();

                if (ImGui::SmallButton(segmentName.c_str()))
                {
                    navigateTo(accumulated);
                }
            }
        }
    }

    auto ContentBrowserWindow::drawFolderTree() -> void
    {
        [[maybe_unused]] ui::ScopedChild folderTree{"FolderTree", ImVec2(m_treePanelWidth, 0), true};

        if (std::filesystem::exists(m_rootPhysical))
        {
            drawFolderTreeNode(m_rootVirtual);
        }
    }

    auto ContentBrowserWindow::drawFolderTreeNode(std::filesystem::path const& path) -> void
    {
        auto name = path.filename().string();
        if (name.empty())
        {
            name = path.string();
        }

        auto physicalPath = resolvePhysicalPath(path);

        auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (m_currentVirtual == path)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        // Check if has subdirectories
        auto hasSubdirs = false;
        if (std::filesystem::exists(physicalPath) && std::filesystem::is_directory(physicalPath))
        {
            for (auto const& entry : std::filesystem::directory_iterator{physicalPath})
            {
                if (entry.is_directory())
                {
                    hasSubdirs = true;
                    break;
                }
            }
        }

        if (!hasSubdirs)
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        auto isOpen = (path == m_rootVirtual);
        auto nodeLabel = std::format("{} {}", isOpen ? ICON_MS_FOLDER_OPEN : ICON_MS_FOLDER, name);
        auto opened = ImGui::TreeNodeEx(path.string().c_str(), flags, "%s", nodeLabel.c_str());

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            navigateTo(path);
        }

        if (opened)
        {
            if (std::filesystem::exists(physicalPath) && std::filesystem::is_directory(physicalPath))
            {
                auto subdirs = std::vector<std::filesystem::path>{};
                for (auto const& entry : std::filesystem::directory_iterator{physicalPath})
                {
                    if (entry.is_directory())
                    {
                        subdirs.push_back(entry.path());
                    }
                }
                std::sort(subdirs.begin(), subdirs.end());

                for (auto const& subdir : subdirs)
                {
                    drawFolderTreeNode(path / subdir.filename());
                }
            }
            ImGui::TreePop();
        }
    }

    auto ContentBrowserWindow::drawContentGrid(EditorContext& context) -> void
    {
        [[maybe_unused]] ui::ScopedChild contentGrid{"ContentGrid", ImVec2(0, 0), true};

        auto availWidth = ImGui::GetContentRegionAvail().x;
        auto itemWidth = m_thumbnailSize + 8.0f;
        auto columns = std::max(1, static_cast<int>(availWidth / itemWidth));

        auto itemIndex = 0;
        for (auto const& item : m_filteredItems)
        {
            if (itemIndex > 0 && itemIndex % columns != 0)
            {
                ImGui::SameLine();
            }

            drawContentItem(context, item, itemWidth);
            ++itemIndex;
        }

        if (m_hasPendingNavigation)
        {
            auto target = m_pendingNavigation;
            m_hasPendingNavigation = false;
            navigateTo(target);
            return;
        }

        // Context menu for empty space
        if (ImGui::BeginPopupContextWindow("ContentBrowserContextMenu", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem(ICON_MS_CREATE_NEW_FOLDER " New Folder"))
            {
                m_showCreateFolderPopup = true;
                m_newFolderName.fill('\0');
            }
            if (ImGui::MenuItem(ICON_MS_REFRESH " Refresh"))
            {
                refreshContentItems();
            }
            if (ImGui::MenuItem(ICON_MS_OPEN_IN_NEW " Show in Explorer"))
            {
                openInExplorer(m_currentPhysical);
            }
            ImGui::EndPopup();
        }

        // Handle create folder popup
        createFolder(context);
    }

    auto ContentBrowserWindow::drawContentItem(EditorContext& context, ContentItem const& item, float itemWidth) -> void
    {
        auto isSelected = m_selectedPaths.contains(item.virtualPath);
        auto icon = getItemIcon(item.type);

        auto itemId = item.virtualPath.string();
        ui::ScopedID itemScope{itemId.c_str()};

        // Calculate item bounds
        auto itemHeight = m_thumbnailSize + ImGui::GetTextLineHeightWithSpacing() + 4.0f;
        auto cursorPos = ImGui::GetCursorScreenPos();
        auto itemMin = cursorPos;
        auto itemMax = ImVec2(cursorPos.x + itemWidth, cursorPos.y + itemHeight);

        // Selection highlight
        if (isSelected)
        {
            auto* drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(itemMin, itemMax, IM_COL32(60, 100, 150, 180), 4.0f);
        }

        // Invisible button for interaction
        ImGui::InvisibleButton("ItemButton", ImVec2(itemWidth, itemHeight));

        auto isHovered = ImGui::IsItemHovered();
        auto isClicked = ImGui::IsItemClicked();
        auto isDoubleClicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0);

        // Hover highlight
        if (isHovered && !isSelected)
        {
            auto* drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(itemMin, itemMax, IM_COL32(80, 80, 80, 100), 4.0f);
        }

        // Draw icon
        auto iconSize = m_thumbnailSize * 0.7f;
        auto iconX = cursorPos.x + (itemWidth - iconSize) * 0.5f;
        auto iconY = cursorPos.y + 4.0f;
        auto iconPos = ImVec2(iconX, iconY);
        auto* drawList = ImGui::GetWindowDrawList();
        auto iconFont = ::april::ui::getIconFont();
        if (!iconFont)
        {
            iconFont = ImGui::GetFont();
        }
        drawList->AddText(iconFont, iconSize, iconPos, ImGui::GetColorU32(ImGuiCol_Text), icon);

        // Draw name
        auto textY = cursorPos.y + m_thumbnailSize;

        // Truncate text to fit
        auto displayName = item.displayName;
        auto textWidth = ImGui::CalcTextSize(displayName.c_str()).x;
        if (textWidth > itemWidth - 4.0f)
        {
            while (displayName.size() > 3 && ImGui::CalcTextSize((displayName + "...").c_str()).x > itemWidth - 4.0f)
            {
                displayName.pop_back();
            }
            displayName += "...";
            textWidth = ImGui::CalcTextSize(displayName.c_str()).x;
        }

        auto textX = cursorPos.x + (itemWidth - textWidth) * 0.5f;
        auto textPos = ImVec2(textX, textY);

        auto isRenaming = m_renaming && m_renamingPath == item.virtualPath;

        if (isRenaming)
        {
            auto inputWidth = itemWidth - 6.0f;
            auto inputX = cursorPos.x + (itemWidth - inputWidth) * 0.5f;
            auto inputPos = ImVec2(inputX, textY);

            ImGui::SetCursorScreenPos(inputPos);
            ImGui::SetNextItemWidth(inputWidth);
            ImGui::SetKeyboardFocusHere();
            bool commit = ImGui::InputText("##Rename", m_renameBuffer.data(), m_renameBuffer.size(),
                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

            if (commit || ImGui::IsItemDeactivatedAfterEdit())
            {
                renameItem(context, item.virtualPath);
            }
            else if (ImGui::IsItemDeactivated())
            {
                m_renaming = false;
                m_renamingPath.clear();
            }

            ImGui::SetCursorScreenPos(cursorPos);
            ImGui::Dummy(ImVec2(itemWidth, itemHeight));
        }
        else
        {
            // Draw label
            drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), textPos, ImGui::GetColorU32(ImGuiCol_Text), displayName.c_str());
        }

        // Handle clicks
        if (isClicked)
        {
            auto ctrlHeld = ImGui::GetIO().KeyCtrl;
            auto shiftHeld = ImGui::GetIO().KeyShift;
            handleSelection(item.virtualPath, ctrlHeld, shiftHeld);
        }

        if (isDoubleClicked)
        {
            if (item.isDirectory)
            {
                m_pendingNavigation = item.virtualPath;
                m_hasPendingNavigation = true;
            }
            else if (item.type == ContentItemType::Mesh)
            {
                spawnMeshAsset(context, item);
            }
        }

        // Tooltip
        if (isHovered)
        {
            ImGui::SetTooltip("%s", item.virtualPath.string().c_str());
        }

        // Context menu
        if (ImGui::BeginPopupContextItem("ItemContextMenu"))
        {
            if (!isSelected)
            {
                m_selectedPaths.clear();
                m_selectedPaths.insert(item.virtualPath);
            }

            if (ImGui::MenuItem(ICON_MS_OPEN_IN_NEW " Open in Explorer"))
            {
                openInExplorer(item.physicalPath);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_MS_EDIT " Rename", "F2"))
            {
                m_renaming = true;
                m_renamingPath = item.virtualPath;
                auto name = item.displayName;
                std::copy(name.begin(), name.end(), m_renameBuffer.begin());
                m_renameBuffer[name.size()] = '\0';
            }
            if (ImGui::MenuItem(ICON_MS_DELETE " Delete", "Delete"))
            {
                m_showDeleteConfirm = true;
                m_deleteTargetPath = item.virtualPath;
            }
            ImGui::EndPopup();
        }

        // Drag source for files
        if (!item.isDirectory && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            auto pathStr = item.virtualPath.string();
            ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pathStr.c_str(), pathStr.size() + 1);
            ImGui::Text("%s %s", icon, item.displayName.c_str());
            ImGui::EndDragDropSource();
        }

    }

    auto ContentBrowserWindow::spawnMeshAsset(EditorContext& context, ContentItem const& item) -> void
    {
        if (item.physicalPath.extension() != ".asset")
        {
            return;
        }

        auto* sceneGraph = Engine::get().getSceneGraph();
        if (!sceneGraph)
        {
            return;
        }

        auto name = item.physicalPath.stem().string();
        if (name.empty())
        {
            name = item.displayName;
        }

        auto entity = sceneGraph->createEntity(name);
        auto& registry = sceneGraph->getRegistry();
        auto& meshRenderer = registry.emplace<scene::MeshRendererComponent>(entity);

        if (auto* resources = Engine::get().getRenderResourceRegistry())
        {
            meshRenderer.meshId = resources->registerMesh(item.physicalPath.string());
        }
        meshRenderer.enabled = true;
        context.selection.entity = entity;
    }

    auto ContentBrowserWindow::drawContextMenu(EditorContext& context) -> void
    {
        // Delete confirmation modal
        if (m_showDeleteConfirm)
        {
            ImGui::OpenPopup("Delete Confirmation");
            m_showDeleteConfirm = false;
        }

        if (ImGui::BeginPopupModal("Delete Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Are you sure you want to delete:");
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "%s", m_deleteTargetPath.filename().string().c_str());
            ImGui::Separator();

            if (ImGui::Button("Delete", ImVec2(120, 0)))
            {
                deleteItem(context, m_deleteTargetPath);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_deleteTargetPath.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    auto ContentBrowserWindow::createFolder(EditorContext& context) -> void
    {
        if (m_showCreateFolderPopup)
        {
            ImGui::OpenPopup("Create Folder");
            m_showCreateFolderPopup = false;
        }

        if (ImGui::BeginPopupModal("Create Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter folder name:");
            ImGui::SetNextItemWidth(250.0f);

            auto enterPressed = ImGui::InputText("##FolderName", m_newFolderName.data(), m_newFolderName.size(),
                ImGuiInputTextFlags_EnterReturnsTrue);

            if (enterPressed || ImGui::Button("Create", ImVec2(120, 0)))
            {
                auto folderName = std::string{m_newFolderName.data()};
                if (!folderName.empty())
                {
                    auto newVirtualPath = m_currentVirtual / folderName;
                    auto newPhysicalPath = resolvePhysicalPath(newVirtualPath);

                    context.commandStack.execute(
                        std::format("Create folder '{}'", folderName),
                        [newPhysicalPath]() {
                            std::filesystem::create_directory(newPhysicalPath);
                        },
                        [newPhysicalPath]() {
                            std::filesystem::remove(newPhysicalPath);
                        }
                    );

                    refreshContentItems();
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    auto ContentBrowserWindow::renameItem(EditorContext& context, std::filesystem::path const& path) -> void
    {
        auto enteredName = std::string{m_renameBuffer.data()};
        auto oldPath = path;
        auto oldPhysicalPath = resolvePhysicalPath(oldPath);
        auto const isDirectory = std::filesystem::is_directory(oldPhysicalPath);

        auto newName = enteredName;
        if (!isDirectory)
        {
            auto const hiddenSuffix = getHiddenSuffix(path, false);
            if (!hiddenSuffix.empty() && !newName.ends_with(hiddenSuffix))
            {
                newName += hiddenSuffix;
            }
        }

        if (newName.empty() || newName == path.filename().string())
        {
            m_renaming = false;
            m_renamingPath.clear();
            return;
        }

        auto newPath = path.parent_path() / newName;
        auto newPhysicalPath = resolvePhysicalPath(newPath);

        context.commandStack.execute(
            std::format("Rename '{}' to '{}'", oldPath.filename().string(), newName),
            [oldPhysicalPath, newPhysicalPath]() {
                if (std::filesystem::exists(oldPhysicalPath))
                {
                    std::filesystem::rename(oldPhysicalPath, newPhysicalPath);
                }
            },
            [oldPhysicalPath, newPhysicalPath]() {
                if (std::filesystem::exists(newPhysicalPath))
                {
                    std::filesystem::rename(newPhysicalPath, oldPhysicalPath);
                }
            }
        );

        m_renaming = false;
        m_renamingPath.clear();

        // Update selection
        if (m_selectedPaths.contains(oldPath))
        {
            m_selectedPaths.erase(oldPath);
            m_selectedPaths.insert(newPath);
        }
        if (m_lastSelectedPath == oldPath)
        {
            m_lastSelectedPath = newPath;
        }

        refreshContentItems();
    }

    auto ContentBrowserWindow::deleteItem(EditorContext&, std::filesystem::path const& path) -> void
    {
        auto physicalPath = resolvePhysicalPath(path);
        if (!std::filesystem::exists(physicalPath))
        {
            m_deleteTargetPath.clear();
            return;
        }

        // Note: Undo for delete is not easily reversible without backup
        // For now, we just delete without undo support
        std::filesystem::remove_all(physicalPath);

        m_selectedPaths.erase(path);
        if (m_lastSelectedPath == path)
        {
            m_lastSelectedPath.clear();
        }
        m_deleteTargetPath.clear();

        refreshContentItems();
    }

    auto ContentBrowserWindow::openInExplorer(std::filesystem::path const& path) -> void
    {
        auto pathStr = path.string();

#if defined(_WIN32)
        auto command = std::format("explorer.exe /select,\"{}\"", pathStr);
        std::system(command.c_str());
#elif defined(__APPLE__)
        auto command = std::format("open -R \"{}\"", pathStr);
        std::system(command.c_str());
#else
        auto parentDir = path.parent_path().string();
        auto command = std::format("xdg-open \"{}\"", parentDir);
        std::system(command.c_str());
#endif
    }

    auto ContentBrowserWindow::onUIRender(EditorContext& context) -> void
    {
        if (!open)
        {
            return;
        }

        m_assetManager = context.assetManager;
        ensureInitialized(context);

        ui::ScopedWindow window{title(), &open};
        if (!window)
        {
            return;
        }

        if (m_refreshPending)
        {
            m_refreshPending = false;
            refreshContentItems();
        }

        // Toolbar
        drawToolbar();
        ImGui::Separator();

        // Breadcrumbs
        drawBreadcrumbs();
        ImGui::Separator();

        // Main content area
        auto availHeight = ImGui::GetContentRegionAvail().y;

        if (m_showFolderTree)
        {
            // Folder tree panel
            drawFolderTree();

            // Splitter
            ImGui::SameLine();
            ImGui::Button("##Splitter", ImVec2(4.0f, availHeight));
            if (ImGui::IsItemActive())
            {
                m_treePanelWidth += ImGui::GetIO().MouseDelta.x;
                m_treePanelWidth = std::clamp(m_treePanelWidth, 100.0f, 400.0f);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }

            ImGui::SameLine();
        }

        // Content grid
        drawContentGrid(context);

        // Handle keyboard shortcuts
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
        {
            m_actionManager.processShortcuts(!m_renaming);
        }

        // Draw modals
        drawContextMenu(context);

    }
}
