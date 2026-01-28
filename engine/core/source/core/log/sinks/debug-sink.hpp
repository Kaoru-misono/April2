#pragma once

#include "../log-sink.hpp"
#include "../logger.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <filesystem>
#include <regex>

namespace april
{
    /**
     * @brief Log sink for IDE debug output. Strips ANSI escape codes.
     */
    class DebugSink : public ILogSink
    {
    public:
        auto log(LogContext const& context, LogConfig const& config, std::string_view message) -> void override
        {
#ifdef _WIN32
            std::string prefix = formatLogPrefix(context, config, false);

            std::string suffix = "";
            if (context.level >= ELogLevel::Warning)
            {
                std::string fileName = std::filesystem::path(context.location.file_name()).filename().string();
                suffix = std::format(" [{}:{}]", fileName, context.location.line());
            }

            std::string cleanMessage = stripAnsi(message);
            std::string fullMessage = prefix + cleanMessage + suffix + "\n";
            OutputDebugStringA(fullMessage.c_str());
#endif
        }

    private:
        auto stripAnsi(std::string_view text) const -> std::string
        {
            static const std::regex ansiRegex("\033\\[[0-9;]*m");
            return std::regex_replace(std::string(text), ansiRegex, "");
        }
    };

} // namespace april
