#include "redis_command_common.h"

#include "redis_command_string.h"

#include <doctest/doctest.h>

#include <string>

namespace {

auto response_to_string(const xin::redis::response& resp) -> std::string
{
    std::string out;
    for (const auto& buffer : resp->to_buffer())
        out.append(static_cast<const char*>(buffer.data()), buffer.size());

    return out;
}

} // namespace

TEST_SUITE("redis-command-common")
{
    using xin::redis::arguments;
    using xin::redis::common_commands;
    using xin::redis::db;
    using xin::redis::string_commands;

    TEST_CASE("ping returns pong or echo")
    {
        CHECK(response_to_string(common_commands::ping(arguments{ "PING" })) == "+PONG\r\n");
        CHECK(response_to_string(common_commands::ping(arguments{ "PING", "hello" })) ==
              "+hello\r\n");
    }

    TEST_CASE("ping with invalid arity returns error")
    {
        CHECK(response_to_string(common_commands::ping(arguments{ "PING", "a", "b" })) ==
              "-ERR wrong number of arguments for 'ping' command\r\n");
    }

    TEST_CASE("mget returns values and nil for missing keys")
    {
        db().flush();
        CHECK(response_to_string(string_commands::set(
                  arguments{ "SET", "__common_mget_k1__", "v1" })) == "+OK\r\n");

        const auto resp = common_commands::mget(
            arguments{ "MGET", "__common_mget_k1__", "__common_mget_missing__" });
        CHECK(response_to_string(resp) == "*2\r\n$2\r\nv1\r\n$-1\r\n");
    }

    TEST_CASE("db()size and flushdb() work")
    {
        db().flush();
        CHECK(response_to_string(common_commands::dbsize(arguments{ "db()SIZE" })) == ":0\r\n");

        CHECK(response_to_string(string_commands::set(
                  arguments{ "SET", "__common_size_k1__", "v1" })) == "+OK\r\n");
        CHECK(response_to_string(common_commands::dbsize(arguments{ "db()SIZE" })) == ":1\r\n");

        CHECK(response_to_string(common_commands::flushdb(arguments{ "FLUSHdb()", "SYNC" })) ==
              "+OK\r\n");
        CHECK(response_to_string(common_commands::dbsize(arguments{ "db()SIZE" })) == ":0\r\n");
    }

    TEST_CASE("expire ttl persist basic path")
    {
        db().flush();
        CHECK(response_to_string(string_commands::set(
                  arguments{ "SET", "__common_ttl_k1__", "v1" })) == "+OK\r\n");

        CHECK(response_to_string(common_commands::expire(
                  arguments{ "EXPIRE", "__common_ttl_k1__", "60" })) == ":1\r\n");

        const auto ttl_resp =
            response_to_string(common_commands::ttl(arguments{ "TTL", "__common_ttl_k1__" }));
        CHECK(ttl_resp[0] == ':');

        CHECK(response_to_string(common_commands::persist(
                  arguments{ "PERSIST", "__common_ttl_k1__" })) == ":1\r\n");
        CHECK(response_to_string(common_commands::ttl(arguments{ "TTL", "__common_ttl_k1__" })) ==
              ":-1\r\n");
    }
}