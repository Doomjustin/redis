#include "redis_command.h"

#include <doctest/doctest.h>

#include <string>

namespace {

auto response_to_string(const xin::redis::ResponsePtr& resp) -> std::string
{
    std::string out;
    for (const auto& buffer : resp->to_buffer())
        out.append(static_cast<const char*>(buffer.data()), buffer.size());

    return out;
}

} // namespace

TEST_SUITE("redis-command")
{
    using xin::redis::Arguments;
    using xin::redis::commands;
    using xin::redis::db;

    TEST_CASE("dispatch 空命令返回错误")
    {
        Arguments args{};
        const auto resp = commands::dispatch(args);

        CHECK(response_to_string(resp) == "-ERR empty command\r\n");
    }

    TEST_CASE("dispatch 未知命令返回错误")
    {
        Arguments args{ "NOPE" };
        const auto resp = commands::dispatch(args);

        CHECK(response_to_string(resp) == "-ERR unknown command 'NOPE'\r\n");
    }

    TEST_CASE("dispatch 命令名大小写不敏感")
    {
        db().flush();

        CHECK(response_to_string(commands::dispatch(Arguments{ "ping" })) == "+PONG\r\n");
        CHECK(response_to_string(commands::dispatch(Arguments{ "PiNg", "echo" })) == "+echo\r\n");
    }

    TEST_CASE("dispatch routes to modules")
    {
        db().flush();

        CHECK(response_to_string(commands::dispatch(Arguments{ "set", "__dispatch_k1__", "v1" })) ==
              "+OK\r\n");
        CHECK(response_to_string(commands::dispatch(Arguments{ "get", "__dispatch_k1__" })) ==
              "$2\r\nv1\r\n");

        CHECK(response_to_string(commands::dispatch(
                  Arguments{ "hset", "__dispatch_h1__", "name", "tom" })) == ":1\r\n");
        CHECK(response_to_string(commands::dispatch(
                  Arguments{ "hget", "__dispatch_h1__", "name" })) == "$3\r\ntom\r\n");

        CHECK(response_to_string(commands::dispatch(
                  Arguments{ "lpush", "__dispatch_l1__", "a", "b" })) == ":2\r\n");
        CHECK(response_to_string(commands::dispatch(Arguments{
                  "lrange", "__dispatch_l1__", "0", "-1" })) == "*2\r\n$1\r\nb\r\n$1\r\na\r\n");
    }
}