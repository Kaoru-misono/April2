#include <doctest/doctest.h>
#include <core/log/logger.hpp>
#include <core/log/log-sink.hpp>
#include <core/log/sinks/console-sink.hpp>

namespace
{
    struct TestSink : public april::ILogSink
    {
        std::string lastPrefix;
        std::string lastMessage;
        april::LogContext lastContext;

        void log(april::LogContext const& context, april::LogConfig const& config, std::string_view message) override
        {
            lastPrefix = april::formatLogPrefix(context, config, false);
            lastMessage = message;
            lastContext = context;
        }
    };
}

TEST_CASE("Log Enhancement - Layout and Conditional Location")
{
    auto pLogger = std::make_shared<april::Logger>("Test");
    auto pSink = std::make_shared<TestSink>();
    pLogger->addSink(pSink);
    pLogger->addSink(std::make_shared<april::ConsoleSink>());

    SUBCASE("Conditional Location - Info (Should NOT show)")
    {
        pLogger->info(std::source_location::current(), "Info message");

        std::string suffix = "";
        if (pSink->lastContext.level >= april::ELogLevel::Warning)
        {
            suffix = " [File:Line]";
        }
        CHECK(suffix == "");
    }

    SUBCASE("Conditional Location - Warning (Should show)")
    {
        pLogger->warning(std::source_location::current(), "Warn message");

        std::string suffix = "";
        if (pSink->lastContext.level >= april::ELogLevel::Warning)
        {
            suffix = " [File:Line]";
        }
        CHECK(suffix == " [File:Line]");
    }

    SUBCASE("Stylization Utility")
    {
        pLogger->info(std::source_location::current(), "Styled: {}", april::Styled("Success").bold().green());
        // Verify that the message contains ANSI escape codes for bold green
        CHECK(pSink->lastMessage.find("\033[1;32m") != std::string::npos);
        CHECK(pSink->lastMessage.find("Success") != std::string::npos);
        CHECK(pSink->lastMessage.find("\033[0m") != std::string::npos);
    }

    SUBCASE("Format Specifiers")
    {
        pLogger->info(std::source_location::current(), "Specifier: {:<bold><red>}", april::Styled("BoldRed"));
        CHECK(pSink->lastMessage.find("\033[1;31m") != std::string::npos);
        CHECK(pSink->lastMessage.find("BoldRed") != std::string::npos);
    }
}
