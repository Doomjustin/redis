#include "base_aof_logger.h"

#include <doctest/doctest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace xin::base;

TEST_CASE("AOFLogger flushes buffered content on destruction")
{
    const auto uniq = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path() /
                      ("xin_aof_logger_" + std::to_string(uniq) + ".aof");

    std::filesystem::remove(path);

    {
        AOFLogger logger{ path };
        const auto result = logger.append("SET key value\r\n");
        REQUIRE(result.has_value());
    }

    std::ifstream input{ path, std::ios::binary };
    REQUIRE(input.is_open());

    const std::string got{ std::istreambuf_iterator<char>{ input },
                           std::istreambuf_iterator<char>{} };
    CHECK_EQ(got, "SET key value\r\n");

    std::filesystem::remove(path);
}

TEST_CASE("AOFLogger keeps all records under concurrent append")
{
    constexpr int kThreads = 4;
    constexpr int kRecordsPerThread = 200;

    const auto uniq = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path() /
                      ("xin_aof_logger_concurrent_" + std::to_string(uniq) + ".aof");

    std::filesystem::remove(path);

    {
        AOFLogger logger{ path };
        std::vector<std::thread> workers;
        std::atomic<bool> all_accepted{ true };
        workers.reserve(kThreads);

        for (int t = 0; t < kThreads; ++t) {
            workers.emplace_back([&logger, &all_accepted, t]() {
                for (int i = 0; i < kRecordsPerThread; ++i) {
                    const auto result =
                        logger.append("T" + std::to_string(t) + "-" + std::to_string(i) + "\n");
                    if (!result.has_value())
                        all_accepted.store(false, std::memory_order_relaxed);
                }
            });
        }

        for (auto& worker : workers)
            worker.join();

        CHECK(all_accepted.load(std::memory_order_relaxed));
    }

    std::ifstream input{ path, std::ios::binary };
    REQUIRE(input.is_open());

    const std::string got{ std::istreambuf_iterator<char>{ input },
                           std::istreambuf_iterator<char>{} };

    std::istringstream stream{ got };
    std::unordered_map<std::string, int> counts;
    std::string line;

    while (std::getline(stream, line)) {
        if (!line.empty())
            ++counts[line];
    }

    const auto expected_total = kThreads * kRecordsPerThread;
    CHECK_EQ(static_cast<int>(counts.size()), expected_total);

    for (int t = 0; t < kThreads; ++t) {
        for (int i = 0; i < kRecordsPerThread; ++i) {
            const std::string key = "T" + std::to_string(t) + "-" + std::to_string(i);
            CHECK_EQ(counts[key], 1);
        }
    }

    std::filesystem::remove(path);
}

TEST_CASE("AOFLogger exposes timestamp and count in last_error")
{
    const auto uniq = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path() /
                      ("xin_aof_missing_dir_" + std::to_string(uniq)) / "appendonly.aof";

    AOFLogger logger{ path };

    std::optional<AofError> err;
    for (int i = 0; i < 100; ++i) {
        err = logger.last_error();
        if (err.has_value())
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    REQUIRE(err.has_value());
    CHECK_FALSE(err->message.empty());
    CHECK_GE(err->count, 1);
    CHECK_LE(err->timestamp, std::chrono::system_clock::now());

    const auto result = logger.append("SET key value\r\n");
    REQUIRE_FALSE(result.has_value());
    CHECK_EQ(result.error(), RejectReason::IOError);
}
