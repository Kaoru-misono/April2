#include "file-dialog.hpp"
#include "core/window/window.hpp"

#ifdef _WIN32

#include <cassert>
#include <filesystem>
#include <shlobj.h>
#include <wrl.h> // Microsoft::WRL::ComPtr
#include <vector>

namespace april::ui
{
    // Unified dialog mode enum (should match header)
    enum class DialogMode
    {
        OpenFile,
        SaveFile,
        OpenFolder
    };

    static auto unifiedDialog(april::Window* window,
                              std::wstring title,
                              std::wstring exts,
                              DialogMode mode,
                              std::filesystem::path const& initialDir = {}) -> std::filesystem::path
    {
        if (!window)
        {
            assert(!"Attempted to call dialog() on null window!");
            return {};
        }
        HWND hwnd = static_cast<HWND>(window->getNativeWindowHandle());

        // Initialize COM for this thread if not already
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        bool needUninit = SUCCEEDED(hr);

        std::filesystem::path result;

        Microsoft::WRL::ComPtr<IFileDialog> pfd;
        if (mode == DialogMode::SaveFile)
            hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
        else
            hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));

        if (SUCCEEDED(hr))
        {
            DWORD dwOptions;
            hr = pfd->GetOptions(&dwOptions);
            if (SUCCEEDED(hr))
            {
                if (mode == DialogMode::OpenFolder)
                    pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
                else
                    pfd->SetOptions(dwOptions | FOS_FORCEFILESYSTEM);

                if (!title.empty())
                    pfd->SetTitle(title.c_str());

                // Set file type filters for file dialogs
                if (mode != DialogMode::OpenFolder && !exts.empty())
                {
                    // exts format: "Text Files|*.txt|All Files|*.*"
                    std::vector<COMDLG_FILTERSPEC> filters;
                    std::vector<std::wstring> filterNames;
                    std::vector<std::wstring> filterSpecs;
                    size_t start = 0;
                    while (start < exts.size())
                    {
                        size_t sep = exts.find(L'|', start);
                        if (sep == std::wstring::npos)
                            break;
                        std::wstring name = exts.substr(start, sep - start);
                        start = sep + 1;
                        sep = exts.find(L'|', start);
                        std::wstring spec = (sep == std::wstring::npos) ? exts.substr(start) : exts.substr(start, sep - start);
                        start = (sep == std::wstring::npos) ? exts.size() : sep + 1;
                        filterNames.push_back(name);
                        filterSpecs.push_back(spec);
                    }
                    for (size_t i = 0; i < filterNames.size(); ++i)
                    {
                        filters.push_back({filterNames[i].c_str(), filterSpecs[i].c_str()});
                    }
                    if (!filters.empty())
                        pfd->SetFileTypes((UINT)filters.size(), filters.data());
                }

                // Set initial directory if provided
                if (!initialDir.empty() && std::filesystem::exists(initialDir))
                {
                    Microsoft::WRL::ComPtr<IShellItem> pFolder;
                    hr = SHCreateItemFromParsingName(initialDir.c_str(), nullptr, IID_PPV_ARGS(&pFolder));
                    if (SUCCEEDED(hr))
                    {
                        pfd->SetFolder(pFolder.Get());
                    }
                }

                // Show the dialog
                hr = pfd->Show(hwnd);
                if (SUCCEEDED(hr))
                {
                    Microsoft::WRL::ComPtr<IShellItem> pItem;
                    hr = pfd->GetResult(&pItem);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszFilePath = nullptr;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                        if (SUCCEEDED(hr))
                        {
                            result = std::filesystem::path(pszFilePath);
                            CoTaskMemFree(pszFilePath);
                        }
                    }
                }
            }
        }

        if (needUninit)
            CoUninitialize();

        return result;
    }

    static auto to_wstring(char const* utf8) -> std::wstring
    {
        if (!utf8)
            return L"";
        return std::filesystem::path(utf8).wstring();
    }

    auto windowOpenFileDialog(april::Window* window, char const* title, char const* exts) -> std::filesystem::path
    {
        return unifiedDialog(window, to_wstring(title), to_wstring(exts), DialogMode::OpenFile);
    }

    auto windowOpenFileDialog(april::Window* window, char const* title, char const* exts, std::filesystem::path& initialDir) -> std::filesystem::path
    {
        std::filesystem::path result = unifiedDialog(window, to_wstring(title),
                                                     to_wstring(exts), DialogMode::OpenFile, initialDir);
        // Update the initial directory to the directory of the selected file
        if (!result.empty())
        {
            initialDir = result.parent_path();
        }
        return result;
    }

    auto windowSaveFileDialog(april::Window* window, char const* title, char const* exts) -> std::filesystem::path
    {
        return unifiedDialog(window, to_wstring(title), to_wstring(exts), DialogMode::SaveFile);
    }

    auto windowOpenFolderDialog(april::Window* window, char const* title) -> std::filesystem::path
    {
        return unifiedDialog(window, to_wstring(title), {}, DialogMode::OpenFolder);
    }

} // namespace april::ui

#else // Not _WIN32

namespace april::ui
{
    auto windowOpenFileDialog(april::Window* window, char const* title, char const* exts) -> std::filesystem::path { return {}; }
    auto windowOpenFileDialog(april::Window* window, char const* title, char const* exts, std::filesystem::path& initialDir) -> std::filesystem::path { return {}; }
    auto windowSaveFileDialog(april::Window* window, char const* title, char const* exts) -> std::filesystem::path { return {}; }
    auto windowOpenFolderDialog(april::Window* window, char const* title) -> std::filesystem::path { return {}; }
}

#endif
