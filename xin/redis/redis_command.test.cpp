#include "redis_command.h"

#include <doctest/doctest.h>

#include <string>

namespace {

auto response_to_string(const xin::redis::commands::response& resp) -> std::string
{
    std::string out;
    for (const auto& buffer : resp->to_buffer())
        out.append(static_cast<const char*>(buffer.data()), buffer.size());

    return out;
}

} // namespace

TEST_SUITE("redis-command")
{
    using xin::redis::commands;

    TEST_CASE("dispatch 空命令返回错误")
    {
        commands::arguments args{};
        const auto resp = commands::dispatch(args);

        CHECK(response_to_string(resp) == "-ERR empty command\r\n");
    }

    TEST_CASE("dispatch 未知命令返回错误")
    {
        commands::arguments args{ "NOPE" };
        const auto resp = commands::dispatch(args);

        CHECK(response_to_string(resp) == "-ERR unknown command 'NOPE'\r\n");
    }

    TEST_CASE("ping 在无参数时返回PONG")
    {
        commands::arguments args{ "PING" };
        const auto resp = commands::dispatch(args);

        CHECK(response_to_string(resp) == "+PONG\r\n");
    }

    TEST_CASE("ping 在单参数时回显参数")
    {
        commands::arguments args{ "PING", "hello" };
        const auto resp = commands::dispatch(args);

        CHECK(response_to_string(resp) == "+hello\r\n");
    }

    TEST_CASE("ping 参数过多返回错误")
    {
        commands::arguments args{ "PING", "a", "b" };
        const auto resp = commands::dispatch(args);

        CHECK(response_to_string(resp) == "-ERR wrong number of arguments for 'ping' command\r\n");
    }

    TEST_CASE("set/get 正常读写")
    {
        commands::arguments set_args{ "SET", "__cmd_test_k1__", "v1" };
        const auto set_resp = commands::dispatch(set_args);
        CHECK(response_to_string(set_resp) == "+OK\r\n");

        commands::arguments get_args{ "GET", "__cmd_test_k1__" };
        const auto get_resp = commands::dispatch(get_args);
        CHECK(response_to_string(get_resp) == "$2\r\nv1\r\n");
    }

    TEST_CASE("get 不存在key返回空bulk字符串")
    {
        commands::arguments args{ "GET", "__cmd_test_missing_key__" };
        const auto resp = commands::dispatch(args);

        CHECK(response_to_string(resp) == "$-1\r\n");
    }

    TEST_CASE("set/get 参数错误返回错误")
    {
        {
            commands::arguments args{ "SET", "only-key" };
            const auto resp = commands::dispatch(args);
            CHECK(response_to_string(resp) ==
                  "-ERR wrong number of arguments for 'set' command\r\n");
        }

        {
            commands::arguments args{ "GET" };
            const auto resp = commands::dispatch(args);
            CHECK(response_to_string(resp) ==
                  "-ERR wrong number of arguments for 'get' command\r\n");
        }
    }

    TEST_CASE("mget 混合命中与未命中返回标准RESP")
    {
        commands::arguments set_args{ "SET", "__cmd_test_mget_k1__", "v1" };
        const auto set_resp = commands::dispatch(set_args);
        CHECK(response_to_string(set_resp) == "+OK\r\n");

        commands::arguments mget_args{ "MGET", "__cmd_test_mget_k1__",
                                       "__cmd_test_mget_missing__" };
        const auto mget_resp = commands::dispatch(mget_args);

        CHECK(response_to_string(mget_resp) == "*2\r\n$2\r\nv1\r\n$-1\r\n");
    }
}