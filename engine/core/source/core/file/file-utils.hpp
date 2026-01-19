#pragma once

#include <filesystem>
#include <string>

namespace april::core
{
    auto getExecutablePath() -> std::filesystem::path;
    auto utf8FromPath(const std::filesystem::path& path) -> std::string;
} // namespace april::core
