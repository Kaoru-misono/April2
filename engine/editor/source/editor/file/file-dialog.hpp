#pragma once

#include <string>
#include <filesystem>

namespace april
{
    struct Window;
}

namespace april::ui
{
    // opens a file chooser dialog and returns the path to the selected file
    auto windowOpenFileDialog(april::Window* window, char const* title, char const* exts) -> std::filesystem::path;

    // opens a file chooser dialog with an initial directory and returns the path to the selected file, and initialDir is updated to the directory of the selected file
    auto windowOpenFileDialog(april::Window* window, char const* title, char const* exts, std::filesystem::path& initialDir) -> std::filesystem::path;

    // opens a file save dialog and returns the path to the saved file
    auto windowSaveFileDialog(april::Window* window, char const* title, char const* exts) -> std::filesystem::path;

    // opens a folder chooser dialog and returns the path to the selected directory
    auto windowOpenFolderDialog(april::Window* window, char const* title) -> std::filesystem::path;

} // namespace april::ui
