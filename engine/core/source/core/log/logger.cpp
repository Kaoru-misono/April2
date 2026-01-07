#include "core/log/logger.hpp"

#include <chrono>
#include <thread>
#include <filesystem>

namespace april
{
    Logger::Logger(std::string const& name, LogConfig const& config)
        : m_name(name), m_config(config)
    {
        if (enum_has_any_flags(m_config.feature, ELogFeature::EnableFile))
        {
            openFile();
        }
    }

    Logger::~Logger()
    {
        if (enum_has_any_flags(m_config.feature, ELogFeature::EnableFile) && m_fileStream.is_open())
        {
            m_fileStream.close();
        }
    }

    auto Logger::buildPrefix(ELogLevel level, bool isCustom) const -> std::string
    {
        std::string prefix;

        if (enum_has_any_flags(m_config.feature, ELogFeature::ShowTime))
        {
            auto now = std::chrono::system_clock::now();
            prefix += std::format("[{:%Y/%m/%d %H:%M:%S}] ", std::chrono::floor<std::chrono::seconds>(now));
        }

        if (enum_has_any_flags(m_config.feature, ELogFeature::ShowThreadId))
        {
            prefix += std::format("[T-{}] ", std::this_thread::get_id());
        }

        if (enum_has_any_flags(m_config.feature, ELogFeature::ShowName))
        {
            prefix += std::format("[{}] ", m_name);
        }

        if (enum_has_any_flags(m_config.feature, ELogFeature::ShowLevel))
        {
            std::string_view levelStr = isCustom ? "LOG" : getLevelString(level);
            prefix += std::format("[{}] ", levelStr);
        }

        return prefix;
    }

    auto Logger::getColorCode(ELogLevel level) const -> std::string_view
    {
        switch (level)
        {
            case ELogLevel::Trace:    return "\033[90m";
            case ELogLevel::Info:     return "\033[32m";
            case ELogLevel::Warn:     return "\033[33m";
            case ELogLevel::Error:    return "\033[31m";
            case ELogLevel::Critical: return "\033[41;37m";
            default:                 return "\033[0m";
        }
    }

    auto Logger::getLevelString(ELogLevel level) const -> std::string_view
    {
        switch (level)
        {
            case ELogLevel::Trace:    return "TRACE";
            case ELogLevel::Info:     return "INFO";
            case ELogLevel::Warn:     return "WARN";
            case ELogLevel::Error:    return "ERROR";
            case ELogLevel::Critical: return "CRITICAL";
            default:                 return "UNKNOWN";
        }
    }

    auto Logger::buildAnsiCode(ELogColor color, ELogStyle style) const -> std::string
    {
        std::string codes = "\033[";
        bool first = true;

        auto append = [&](uint8_t code) {
            if (!first) codes += ";";
            codes += std::to_string(code);
            first = false;
        };

        if (enum_has_any_flags(style, ELogStyle::Bold))      append(1);
        if (enum_has_any_flags(style, ELogStyle::Dim))       append(2);
        if (enum_has_any_flags(style, ELogStyle::Italic))    append(3);
        if (enum_has_any_flags(style, ELogStyle::Underline)) append(4);
        if (enum_has_any_flags(style, ELogStyle::Blink))     append(5);
        if (enum_has_any_flags(style, ELogStyle::Reverse))   append(7);
        if (enum_has_any_flags(style, ELogStyle::Hidden))    append(8);

        if (!first) codes += ";";
        codes += std::to_string(static_cast<uint8_t>(color));
        codes += "m";

        return codes;
    }

    auto Logger::openFile() -> void
    {
        std::filesystem::path path(m_config.filePath);
        if (path.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
        }

        m_fileStream.open(path, std::ios::out);

        if (!m_fileStream.is_open())
        {
            std::println("[Logger Error] Failed to open log file: {}", m_config.filePath);
        }
    }

    auto Log::getLogger() -> std::shared_ptr<Logger>
    {
        static auto s_Logger = std::make_shared<Logger>("Core");

        return s_Logger;
    }

} // namespace april
