#pragma once

#include "log-types.hpp"

#include <string_view>

namespace april
{
    /**
     * @brief Interface for log sinks.
     */
    class ILogSink
    {
    public:
        virtual ~ILogSink() = default;

        /**
         * @brief Log a message.
         * @param context The log context.
         * @param config The current log config.
         * @param message The log message.
         */
        virtual auto log(LogContext const& context, LogConfig const& config, std::string_view message) -> void = 0;
    };
}
