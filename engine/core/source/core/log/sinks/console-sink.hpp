#pragma once

#include "../log-sink.hpp"
#include "../logger.hpp"

#include <iostream>
#include <print>
#include <filesystem>

namespace april
{
    /**
     * @brief Log sink for console output.
     */
    class ConsoleSink : public ILogSink
    {
    public:
        auto log(LogContext const& context, LogConfig const& config, std::string_view message) -> void override
        {
            std::ostream& target = (context.level >= ELogLevel::Error) ? std::cerr : std::cout;

            std::string prefix = formatLogPrefix(context, config, m_useColor);

            std::string levelColor = m_useColor ? std::string(getColorCode(context.level)) : "";
            std::string resetColor = m_useColor ? "\033[0m" : "";

            std::string suffix = "";
            if (context.level >= ELogLevel::Warning)
            {
                std::string fileName = std::filesystem::path(context.location.file_name()).filename().string();
                suffix = std::format(" [{}:{}]", fileName, context.location.line());
            }

            std::println(target, "{}{}{}{}{}", levelColor, prefix, message, suffix, resetColor);
        }

        auto setUseColor(bool use) -> void { m_useColor = use; }

    private:
        auto getColorCode(ELogLevel level) const -> std::string_view
        {
            switch (level)
            {
                case ELogLevel::Trace:
                    return "\033[90m"; // Gray
                case ELogLevel::Debug:
                    return "\033[36m"; // Cyan
                case ELogLevel::Info:
                    return "\033[32m"; // Green
                case ELogLevel::Warning:
                    return "\033[33m"; // Yellow
                case ELogLevel::Error:
                    return "\033[31m"; // Red
                case ELogLevel::Fatal:
                    return "\033[41;37m"; // White on Red background
                default:
                    return "\033[0m";
            }
        }

    private:
        bool m_useColor{true};
    };

} // namespace april
