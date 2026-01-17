#pragma once

#include "../tools/enum-flags.hpp"

#include <cstdint>
#include <string>
#include <format>
#include <print>
#include <memory>
#include <mutex>
#include <iostream>
#include <fstream>
#include <functional>
#include <vector>

namespace april
{
    enum struct ELogLevel: uint8_t
    {
        Trace = 0,
        Info,
        Warn,
        Error,
        Critical,
    };

    enum struct ELogColor: uint8_t
    {
        // Standard Colors (0-7)
        Black   = 30,
        Red     = 31,
        Green   = 32,
        Yellow  = 33,
        Blue    = 34,
        Magenta = 35,
        Cyan    = 36,
        White   = 37,

        // Default
        Default = 39,

        // High Intensity / Bright Colors (8-15)
        BrightBlack   = 90,
        BrightRed     = 91,
        BrightGreen   = 92,
        BrightYellow  = 93,
        BrightBlue    = 94,
        BrightMagenta = 95,
        BrightCyan    = 96,
        BrightWhite   = 97,
    };

    enum struct ELogStyle: uint8_t
    {
        None      = 0,
        Bold      = 1 << 0,
        Dim       = 1 << 1,
        Italic    = 1 << 2,
        Underline = 1 << 3,
        Blink     = 1 << 4,
        Reverse   = 1 << 5,
        Hidden    = 1 << 6,
    };
    AP_ENUM_CLASS_OPERATORS(ELogStyle);

    enum struct ELogFeature: uint8_t
    {
        None = 0,
        EnableConsole = 1 << 0,
        EnableFile    = 1 << 1,
        EnableColor   = 1 << 2,
        ShowName      = 1 << 3,
        ShowLevel     = 1 << 4,
        ShowTime      = 1 << 5,
        ShowThreadId  = 1 << 6,
    };
    AP_ENUM_CLASS_OPERATORS(ELogFeature);

    static constexpr ELogFeature DefaultLogFeatures = ELogFeature::EnableConsole | ELogFeature::EnableColor | ELogFeature::ShowLevel | ELogFeature::ShowTime;

    struct LogConfig
    {
        ELogFeature feature{DefaultLogFeatures};

        std::string filePath{"log.txt"};
    };

    struct Logger
    {
    public:
        using LogSinkFn = std::function<void(ELogLevel level, std::string const& prefix, std::string const& message)>;
        using LogSinkId = uint32_t;

        Logger(std::string const& name, LogConfig const& config = {});
        ~Logger();

        template <typename... Args>
        auto trace(std::format_string<Args...> fmt, Args&&... args) -> void
        {
            log(ELogLevel::Trace, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto info(std::format_string<Args...> fmt, Args&&... args) -> void
        {
            log(ELogLevel::Info, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto warn(std::format_string<Args...> fmt, Args&&... args) -> void
        {
            log(ELogLevel::Warn, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto error(std::format_string<Args...> fmt, Args&&... args) -> void
        {
            log(ELogLevel::Error, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto critical(std::format_string<Args...> fmt, Args&&... args) -> void
        {
            log(ELogLevel::Critical, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto customLog(ELogColor color, ELogStyle style, std::format_string<Args...> fmt, Args&&... args) -> void
        {
            if (m_minLevel > ELogLevel::Info) return;

            auto message = std::format(fmt, std::forward<Args>(args)...);
            auto prefix = buildPrefix(ELogLevel::Info, true);

            std::vector<std::pair<LogSinkId, LogSinkFn>> sinksCopy;
            {
                std::lock_guard<std::recursive_mutex> lock(m_mutex);

                if (enum_has_any_flags(m_config.feature, ELogFeature::EnableConsole))
                {
                    if (enum_has_any_flags(m_config.feature, ELogFeature::EnableColor))
                    {
                        auto ansiCode = buildAnsiCode(color, style);
                        std::println("{}{}{}\033[0m", ansiCode, prefix, message);
                    }
                    else
                    {
                        std::println("{}{}", prefix, message);
                    }
                }

                if (enum_has_any_flags(m_config.feature, ELogFeature::EnableFile) && m_fileStream.is_open())
                {
                    std::println(m_fileStream, "{}{}", prefix, message);
                }

                sinksCopy = m_sinks;
            }

            for (auto const& sink : sinksCopy)
            {
                sink.second(ELogLevel::Info, prefix, message);
            }
        }

        auto setLevel(ELogLevel level) -> void { m_minLevel = level; }
        [[nodiscard]] auto getConfig() -> LogConfig& { return m_config; }

        auto addSink(LogSinkFn sink) -> LogSinkId
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            LogSinkId id = m_nextSinkId++;
            m_sinks.push_back({id, std::move(sink)});
            return id;
        }

        auto removeSink(LogSinkId id) -> void
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            std::erase_if(m_sinks, [id](auto const& pair) { return pair.first == id; });
        }

    private:
        template <typename... Args>
        auto log(ELogLevel level, std::format_string<Args...> fmt, Args&&... args) -> void
        {
            if (level < m_minLevel) return;

            auto message = std::format(fmt, std::forward<Args>(args)...);
            auto prefix = buildPrefix(level);

            std::vector<std::pair<LogSinkId, LogSinkFn>> sinksCopy;
            {
                std::lock_guard<std::recursive_mutex> lock(m_mutex);

                if (enum_has_any_flags(m_config.feature, ELogFeature::EnableConsole))
                {
                    std::ostream& targetStream = (level == ELogLevel::Critical) ? std::cerr : std::cout;

                    if (enum_has_any_flags(m_config.feature, ELogFeature::EnableColor))
                    {
                        std::println(targetStream, "{}{}{}\033[0m", getColorCode(level), prefix, message);
                    }
                    else
                    {
                        std::println(targetStream, "{}{}", prefix, message);
                    }
                }

                // 2. File Output
                if (enum_has_any_flags(m_config.feature, ELogFeature::EnableFile) && m_fileStream.is_open())
                {
                    std::println(m_fileStream, "{}{}", prefix, message);

                    if (level == ELogLevel::Critical)
                    {
                        m_fileStream.flush();
                    }
                }

                sinksCopy = m_sinks;
            }

            for (auto const& sink : sinksCopy)
            {
                sink.second(level, prefix, message);
            }
        }

        [[nodiscard]] auto buildPrefix(ELogLevel level, bool isCustom = false) const -> std::string;
        [[nodiscard]] auto getColorCode(ELogLevel level) const -> std::string_view;
        [[nodiscard]] auto getLevelString(ELogLevel level) const -> std::string_view;
        [[nodiscard]] auto buildAnsiCode(ELogColor color, ELogStyle style) const -> std::string;

        auto openFile() -> void;

    private:
        std::string m_name{};
        LogConfig m_config{};
        ELogLevel m_minLevel{ELogLevel::Trace};
        std::recursive_mutex m_mutex{};
        std::ofstream m_fileStream{};
        std::vector<std::pair<LogSinkId, LogSinkFn>> m_sinks;
        LogSinkId m_nextSinkId{0};
    };

    struct Log
    {
        [[nodiscard]] static auto getLogger() -> std::shared_ptr<Logger>;
    };

} // namespace april

#define AP_TRACE(...)        ::april::Log::getLogger()->trace(__VA_ARGS__)
#define AP_INFO(...)         ::april::Log::getLogger()->info(__VA_ARGS__)
#define AP_WARN(...)         ::april::Log::getLogger()->warn(__VA_ARGS__)
#define AP_ERROR(...)        ::april::Log::getLogger()->error(__VA_ARGS__)
#define AP_CRITICAL(...)     ::april::Log::getLogger()->critical(__VA_ARGS__)
#define AP_LOG(color, style, ...) ::april::Log::getLogger()->customLog(color, style, __VA_ARGS__)
