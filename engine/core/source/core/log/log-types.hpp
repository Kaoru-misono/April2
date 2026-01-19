#pragma once

#include <cstdint>
#include "../tools/enum-flags.hpp"

#include <string>
#include <chrono>
#include <thread>
#include <source_location>

namespace april
{
    /**
     * @brief Configuration for the logger.
     */
    struct LogConfig
    {
        std::string filePath{"log.txt"};
        bool        showTime{true};
        bool        showName{false};
        bool        showLevel{true};
        bool        showThreadID{false};
    };

    /**
     * @brief Log severity levels.
     */
    enum struct ELogLevel : uint8_t
    {
        Trace = 0,
        Debug,
        Info,
        Warning,
        Error,
        Fatal,
    };

    /**
     * @brief Log colors for console output.
     */
    enum struct ELogColor : uint8_t
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
        Gray          = 90,
        BrightRed     = 91,
        BrightGreen   = 92,
        BrightYellow  = 93,
        BrightBlue    = 94,
        BrightMagenta = 95,
        BrightCyan    = 96,
        BrightWhite   = 97,
    };

    /**
     * @brief Log styles for console output.
     */
    enum struct ELogStyle : uint8_t
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

    /**
     * @brief Context information for a log entry.
     */
    struct LogContext
    {
        using TimeStamp = std::chrono::system_clock::time_point;

        ELogLevel            level;
        std::string          name;
        std::source_location location;
        TimeStamp            timestamp;
        std::thread::id      threadID;
    };

} // namespace april
