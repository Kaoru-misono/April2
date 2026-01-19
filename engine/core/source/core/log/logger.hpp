#pragma once

#include "log-style.hpp"
#include "log-sink.hpp"

#include <string>
#include <format>
#include <memory>
#include <mutex>
#include <vector>
#include <source_location>

namespace april
{
    /**
     * @brief The core logger class.
     */
    class Logger
    {
    public:
        Logger(std::string const& name, LogConfig const& config = {});
        ~Logger();

        template <typename... Args>
        auto trace(std::source_location const& loc, std::format_string<Args...> fmt, Args&&... args) -> void
        {
            log(ELogLevel::Trace, loc, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto debug(std::source_location const& loc, std::format_string<Args...> fmt, Args&&... args) -> void
        {
            log(ELogLevel::Debug, loc, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto info(std::source_location const& loc, std::format_string<Args...> fmt, Args&&... args) -> void
        {
            log(ELogLevel::Info, loc, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto warning(std::source_location const& loc, std::format_string<Args...> fmt, Args&&... args) -> void
        {
            log(ELogLevel::Warning, loc, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto error(std::source_location const& loc, std::format_string<Args...> fmt, Args&&... args) -> void
        {
            log(ELogLevel::Error, loc, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto fatal(std::source_location const& loc, std::format_string<Args...> fmt, Args&&... args) -> void
        {
            log(ELogLevel::Fatal, loc, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Backward compatibility for critical level.
         */
        template <typename... Args>
        auto critical(std::source_location const& loc, std::format_string<Args...> fmt, Args&&... args) -> void
        {
            log(ELogLevel::Fatal, loc, fmt, std::forward<Args>(args)...);
        }

        auto addSink(std::shared_ptr<ILogSink> p_sink) -> void;
        auto removeSink(std::shared_ptr<ILogSink> p_sink) -> void;

        auto setLevel(ELogLevel level) -> void { m_minLevel = level; }

        auto setConfig(LogConfig const& config) -> void { m_config = config; }
        auto getConfig() const -> LogConfig const& { return m_config; }

    private:
        template <typename... Args>
        auto log(ELogLevel level, std::source_location const& loc, std::format_string<Args...> fmt, Args&&... args) -> void
        {
            if (level < m_minLevel)
            {
                return;
            }

            auto message = std::format(fmt, std::forward<Args>(args)...);

            auto context = LogContext{
                .level = level,
                .name = m_name,
                .location = loc,
                .timestamp = std::chrono::system_clock::now(),
                .threadID = std::this_thread::get_id()
            };

            auto sinksCopy = std::vector<std::shared_ptr<ILogSink>>{};
            {
                std::lock_guard<std::recursive_mutex> lock(m_mutex);
                sinksCopy = m_sinks;
            }

            for (auto const& p_sink : sinksCopy)
            {
                p_sink->log(context, m_config, message);
            }
        }

    private:
        std::string                            m_name{};
        LogConfig                              m_config{};
        ELogLevel                              m_minLevel{ELogLevel::Trace};
        mutable std::recursive_mutex           m_mutex{};
        std::vector<std::shared_ptr<ILogSink>> m_sinks{};
    };

    /**
     * @brief Helper to format a log prefix based on config.
     */
    auto formatLogPrefix(LogContext const& context, LogConfig const& config, bool useColor = false) -> std::string;

    /**
     * @brief Access to the global logger.
     */
    struct Log
    {
        static auto getLogger() -> std::shared_ptr<Logger>;
    };

} // namespace april

#define AP_TRACE(...)    ::april::Log::getLogger()->trace(std::source_location::current(), __VA_ARGS__)
#define AP_DEBUG(...)    ::april::Log::getLogger()->debug(std::source_location::current(), __VA_ARGS__)
#define AP_INFO(...)     ::april::Log::getLogger()->info(std::source_location::current(), __VA_ARGS__)
#define AP_WARN(...)     ::april::Log::getLogger()->warning(std::source_location::current(), __VA_ARGS__)
#define AP_ERROR(...)    ::april::Log::getLogger()->error(std::source_location::current(), __VA_ARGS__)
#define AP_FATAL(...)    ::april::Log::getLogger()->fatal(std::source_location::current(), __VA_ARGS__)
#define AP_CRITICAL(...) ::april::Log::getLogger()->critical(std::source_location::current(), __VA_ARGS__)
