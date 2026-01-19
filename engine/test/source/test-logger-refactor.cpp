#include <doctest/doctest.h>
#include "core/log/logger.hpp"
#include "core/log/log-sink.hpp"
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>

namespace
{
    struct MockSink : public april::ILogSink
    {
        std::atomic<int> callCount{0};
        std::mutex mtx;
        std::vector<std::string> messages;

        void log(april::LogContext const& context, april::LogConfig const& config, std::string_view message) override
        {
            callCount++;
            std::lock_guard<std::mutex> lock(mtx);
            messages.push_back(std::string(message));
        }
    };
}

TEST_CASE("Logger Refactor - Thread Safety and Modern API")
{
    auto logger = std::make_shared<april::Logger>("TestLogger");
    auto sink = std::make_shared<MockSink>();
    logger->addSink(sink);

    SUBCASE("Basic Logging and Source Location")
    {
        // We use explicit calls here to verify the API, but we must pass source_location
        logger->info(std::source_location::current(), "Hello {}!", "World");
        CHECK(sink->callCount == 1);
        CHECK(sink->messages.back() == "Hello World!");
    }

    SUBCASE("Thread Safety")
    {
        const int numThreads = 10;
        const int logsPerThread = 100;
        std::vector<std::thread> threads;

        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([&logger, i]() {
                for (int j = 0; j < logsPerThread; ++j)
                {
                    logger->debug(std::source_location::current(), "Thread {} log {}", i, j);
                }
            });
        }

        for (auto& t : threads)
        {
            t.join();
        }

        CHECK(sink->callCount == numThreads * logsPerThread);
    }
}