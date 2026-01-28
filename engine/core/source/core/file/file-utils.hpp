#pragma once

#include <filesystem>
#include <string>

namespace april::core
{
    auto getExecutablePath() -> std::filesystem::path;
    auto utf8FromPath(std::filesystem::path const& path) -> std::string;
} // namespace april::core
