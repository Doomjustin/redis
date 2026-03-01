#include "redis_serialization_protocl.h"

#include <doctest/doctest.h>

#include <span>
#include <string_view>

TEST_SUITE("redis-serialization-protocol")
{
    using namespace xin::redis;

    TEST_CASE("RESPParser解析完整命令")
    {
        RESPParser parser;
        constexpr std::string_view req = "*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n";
        std::span<const char> buf{ req.data(), req.size() };

        auto result = parser.parse(buf);

        REQUIRE(result.has_value());
        REQUIRE(result->size() == 2);
        CHECK(result->at(0) == "GET");
        CHECK(result->at(1) == "key");
        CHECK(buf.empty());
    }

    TEST_CASE("RESPParser增量解析半包后补齐成功")
    {
        RESPParser parser;

        constexpr std::string_view partial = "*2\r\n$3\r\nGET\r\n$3\r\n";
        std::span<const char> partial_buf{ partial.data(), partial.size() };
        auto partial_result = parser.parse(partial_buf);

        REQUIRE_FALSE(partial_result.has_value());
        CHECK(partial_result.error() == RESPParser::Error::Waiting);
        CHECK(partial_buf.empty());

        constexpr std::string_view remain = "key\r\n";
        std::span<const char> remain_buf{ remain.data(), remain.size() };
        auto final_result = parser.parse(remain_buf);

        REQUIRE(final_result.has_value());
        REQUIRE(final_result->size() == 2);
        CHECK(final_result->at(0) == "GET");
        CHECK(final_result->at(1) == "key");
        CHECK(remain_buf.empty());
    }

    TEST_CASE("RESPParser在非法起始前缀时返回Error")
    {
        RESPParser parser;
        constexpr std::string_view req = "+OK\r\n";
        std::span<const char> buf{ req.data(), req.size() };

        auto result = parser.parse(buf);

        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == RESPParser::Error::Error);
    }

    TEST_CASE("RESPParser在bulk数据后缀非CRLF时返回Error")
    {
        RESPParser parser;
        constexpr std::string_view req = "*1\r\n$3\r\nabc\rX";
        std::span<const char> buf{ req.data(), req.size() };

        auto result = parser.parse(buf);

        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == RESPParser::Error::Error);
    }

    TEST_CASE("RESPParser reset后可复用解析下一条命令")
    {
        RESPParser parser;

        constexpr std::string_view req1 = "*1\r\n$4\r\nPING\r\n";
        std::span<const char> buf1{ req1.data(), req1.size() };
        auto res1 = parser.parse(buf1);
        REQUIRE(res1.has_value());
        REQUIRE(res1->size() == 1);
        CHECK(res1->at(0) == "PING");
        CHECK(buf1.empty());

        parser.reset();

        constexpr std::string_view req2 = "*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n";
        std::span<const char> buf2{ req2.data(), req2.size() };
        auto res2 = parser.parse(buf2);
        REQUIRE(res2.has_value());
        REQUIRE(res2->size() == 2);
        CHECK(res2->at(0) == "GET");
        CHECK(res2->at(1) == "key");
        CHECK(buf2.empty());
    }
}
