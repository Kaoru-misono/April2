#include <doctest/doctest.h>
#include "core/log/log-types.hpp"
#include "core/log/log-sink.hpp"

TEST_CASE("Log Types and Interface Foundation") {
    SUBCASE("LogLevel Enum Values") {
        CHECK(static_cast<uint8_t>(april::ELogLevel::Trace) == 0);
        CHECK(static_cast<uint8_t>(april::ELogLevel::Debug) == 1);
        CHECK(static_cast<uint8_t>(april::ELogLevel::Info) == 2);
        CHECK(static_cast<uint8_t>(april::ELogLevel::Warning) == 3);
        CHECK(static_cast<uint8_t>(april::ELogLevel::Error) == 4);
        CHECK(static_cast<uint8_t>(april::ELogLevel::Fatal) == 5);
    }

    SUBCASE("LogColor Enum") {
        CHECK(static_cast<uint8_t>(april::ELogColor::Red) == 31);
        CHECK(static_cast<uint8_t>(april::ELogColor::Default) == 39);
    }

    SUBCASE("LogStyle Enum Flags") {
        using namespace april;
        CHECK((ELogStyle::Bold | ELogStyle::Italic) != ELogStyle::None);
        CHECK(enum_has_any_flags(ELogStyle::Bold | ELogStyle::Italic, ELogStyle::Bold));
    }

    // Test that ILogSink is a polymorphic interface
    struct MockSink : public april::ILogSink {
        void log(april::LogContext const& context, april::LogConfig const& config, std::string_view message) override {}
    };

    SUBCASE("ILogSink Interface") {
        MockSink sink;
        april::ILogSink* sinkPtr = &sink;
        CHECK(sinkPtr != nullptr);
    }
}
