#pragma once

#include "../log-sink.hpp"
#include "../logger.hpp"

#include <fstream>
#include <filesystem>
#include <regex>

namespace april
{
    /**
     * @brief Log sink for file output. Strips ANSI escape codes.
     */
    class FileSink : public ILogSink
    {
    public:
        FileSink(std::filesystem::path const& path)
        {
            if (path.has_parent_path())
            {
                std::filesystem::create_directories(path.parent_path());
            }
            m_file.open(path, std::ios::out | std::ios::app);
        }

        auto log(LogContext const& context, LogConfig const& config, std::string_view message) -> void override
        {
            if (m_file.is_open())
            {
                std::string prefix = formatLogPrefix(context, config, false);

                std::string suffix = "";
                if (context.level >= ELogLevel::Warning)
                {
                    std::string fileName = std::filesystem::path(context.location.file_name()).filename().string();
                    suffix = std::format(" [{}:{}]", fileName, context.location.line());
                }

                std::string cleanMessage = stripAnsi(message);
                m_file << std::format("{}{}{}\n", prefix, cleanMessage, suffix);

                if (context.level >= ELogLevel::Error)
                {
                    m_file.flush();
                }
            }
        }

    private:
        auto stripAnsi(std::string_view text) const -> std::string
        {
            static const std::regex ansiRegex("\033\\[[0-9;]*m");
            return std::regex_replace(std::string(text), ansiRegex, "");
        }

    private:
        std::ofstream m_file;
    };

} // namespace april