#include "file-utils.hpp"
#include <windows.h>

namespace april::core
{
    auto getExecutablePath() -> std::filesystem::path
    {
        wchar_t path[MAX_PATH] = {0};
        GetModuleFileNameW(NULL, path, MAX_PATH);
        return std::filesystem::path(path);
    }

    auto utf8FromPath(std::filesystem::path const& path) -> std::string
    {
        return path.string();
    }
} // namespace april::core
