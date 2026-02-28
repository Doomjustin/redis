#include "base_log.h"

#include <doctest/doctest.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace xin::base;

namespace {

struct Record {
    LogLevel level;
    std::string message;
};

class MockLogger final : public Logger {
public:
    explicit MockLogger(std::shared_ptr<std::vector<Record>> sink)
        : sink_(std::move(sink))
    {
    }

    [[nodiscard]]
    auto records() const noexcept -> const std::vector<Record>&
    {
        return *sink_;
    }

private:
    std::shared_ptr<std::vector<Record>> sink_;

    void log(LogLevel level, std::string_view message) override
    {
        sink_->push_back({ level, std::string{ message } });
    }
};

auto install_mock_logger(std::shared_ptr<std::vector<Record>> sink =
                             std::make_shared<std::vector<Record>>()) -> MockLogger *
{
    auto logger = std::make_unique<MockLogger>(std::move(sink));
    auto *logger_ptr = logger.get();
    log::set_default_logger(std::move(logger));
    return logger_ptr;
}

} // namespace

TEST_SUITE("base-log")
{
    TEST_CASE("set_level和level在默认logger上行为一致")
    {
        auto *logger_ptr = install_mock_logger();

        log::set_level(LogLevel::Debug);
        REQUIRE(log::level() == LogLevel::Debug);
        REQUIRE(logger_ptr->level() == LogLevel::Debug);

        log::set_level(LogLevel::Warning);
        REQUIRE(log::level() == LogLevel::Warning);
        REQUIRE(logger_ptr->level() == LogLevel::Warning);
    }

    TEST_CASE("trace会格式化并写入默认logger")
    {
        auto *logger_ptr = install_mock_logger();
        log::trace("trace {}", 1);
        REQUIRE(logger_ptr->records().size() == 1);
        REQUIRE(logger_ptr->records()[0].level == LogLevel::Trace);
        REQUIRE(logger_ptr->records()[0].message == "trace 1");
    }

    TEST_CASE("debug会格式化并写入默认logger")
    {
        auto *logger_ptr = install_mock_logger();
        log::debug("debug {}", 2);
        REQUIRE(logger_ptr->records().size() == 1);
        REQUIRE(logger_ptr->records()[0].level == LogLevel::Debug);
        REQUIRE(logger_ptr->records()[0].message == "debug 2");
    }

    TEST_CASE("info会格式化并写入默认logger")
    {
        auto *logger_ptr = install_mock_logger();
        log::info("info {}", 3);
        REQUIRE(logger_ptr->records().size() == 1);
        REQUIRE(logger_ptr->records()[0].level == LogLevel::Info);
        REQUIRE(logger_ptr->records()[0].message == "info 3");
    }

    TEST_CASE("warning会格式化并写入默认logger")
    {
        auto *logger_ptr = install_mock_logger();
        log::warning("warning {}", 4);
        REQUIRE(logger_ptr->records().size() == 1);
        REQUIRE(logger_ptr->records()[0].level == LogLevel::Warning);
        REQUIRE(logger_ptr->records()[0].message == "warning 4");
    }

    TEST_CASE("error会格式化并写入默认logger")
    {
        auto *logger_ptr = install_mock_logger();
        log::error("error {}", 5);
        REQUIRE(logger_ptr->records().size() == 1);
        REQUIRE(logger_ptr->records()[0].level == LogLevel::Error);
        REQUIRE(logger_ptr->records()[0].message == "error 5");
    }

    TEST_CASE("critical会格式化并写入默认logger")
    {
        auto *logger_ptr = install_mock_logger();
        log::critical("critical {}", 6);
        REQUIRE(logger_ptr->records().size() == 1);
        REQUIRE(logger_ptr->records()[0].level == LogLevel::Critical);
        REQUIRE(logger_ptr->records()[0].message == "critical 6");
    }

    TEST_CASE("替换默认logger后仅写入新logger")
    {
        auto sink_a = std::make_shared<std::vector<Record>>();
        install_mock_logger(sink_a);
        log::info("from-a");
        REQUIRE(sink_a->size() == 1);
        REQUIRE((*sink_a)[0].message == "from-a");

        auto sink_b = std::make_shared<std::vector<Record>>();
        install_mock_logger(sink_b);
        log::info("from-b");

        REQUIRE(sink_a->size() == 1);
        REQUIRE(sink_b->size() == 1);
        REQUIRE((*sink_b)[0].message == "from-b");
    }
}
