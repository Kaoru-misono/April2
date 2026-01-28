#include "logger.hpp"
#include "sinks/console-sink.hpp"
#include "sinks/file-sink.hpp"
#include "sinks/debug-sink.hpp"

#include <chrono>
#include <thread>
#include <sstream>

namespace april
{
    inline namespace
    {
        auto getLevelString(ELogLevel level) -> std::string_view
        {
            switch (level)
            {
                case ELogLevel::Trace:
                    return "TRACE";
                case ELogLevel::Debug:
                    return "DEBUG";
                case ELogLevel::Info:
                    return "INFO";
                case ELogLevel::Warning:
                    return "WARN";
                case ELogLevel::Error:
                    return "ERROR";
                case ELogLevel::Fatal:
                    return "FATAL";
                default:
                    return "UNKNOWN";
            }
        }
    }

    Logger::Logger(std::string const& name, LogConfig const& config)
        : m_name(name)
        , m_config(config)
    {
    }

    Logger::~Logger()
    {
    }

    auto Logger::addSink(std::shared_ptr<ILogSink> p_sink) -> void
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_sinks.push_back(p_sink);
    }

    auto Logger::removeSink(std::shared_ptr<ILogSink> p_sink) -> void
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        std::erase(m_sinks, p_sink);
    }

    auto formatLogPrefix(LogContext const& context, LogConfig const& config, bool useColor) -> std::string
    {
        std::stringstream ss;

        auto appendComponent = [&](std::string const& value)
        {
            if (useColor)
            {
                ss << "\033[1m[" << value << "]\033[22m ";
            }
            else
            {
                ss << "[" << value << "] ";
            }
        };

        if (config.showTime)
        {
            auto const timeZone = std::chrono::current_zone();
            std::chrono::zoned_time localTime{timeZone, std::chrono::floor<std::chrono::seconds>(context.timestamp)};
            auto timeStr = std::format("{:%Y/%m/%d %H:%M:%S}", localTime);
            appendComponent(timeStr);
        }

        if (config.showName)
        {
            appendComponent(context.name);
        }

        if (config.showLevel)
        {
            appendComponent(std::string(getLevelString(context.level)));
        }

        if (config.showThreadID)
        {
            std::stringstream ts;
            ts << "TID:" << context.threadID;
            appendComponent(ts.str());
        }

        return ss.str();
    }

    auto Log::getLogger() -> std::shared_ptr<Logger>
    {
        static auto s_pLogger = []()
        {
            auto p_logger = std::make_shared<Logger>("Core");
            p_logger->addSink(std::make_shared<ConsoleSink>());
            p_logger->addSink(std::make_shared<FileSink>("logs/april.log"));
            p_logger->addSink(std::make_shared<DebugSink>());
            return p_logger;
        }();

        return s_pLogger;
    }

} // namespace april
