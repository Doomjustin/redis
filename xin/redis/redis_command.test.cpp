#include "redis_command.h"

#include <doctest/doctest.h>

#include <string>

namespace {

auto starts_with(const std::string& value, const std::string& prefix) -> bool
{
    return value.rfind(prefix, 0) == 0;
}

} // namespace

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

    TEST_CASE("persist 在有过期和无过期时返回正确整数")
    {
        {
            commands::arguments set_ex{ "SET", "__cmd_test_persist_k1__", "v1", "EX", "60" };
            CHECK(response_to_string(commands::dispatch(set_ex)) == "+OK\r\n");

            commands::arguments persist{ "PERSIST", "__cmd_test_persist_k1__" };
            CHECK(response_to_string(commands::dispatch(persist)) == ":1\r\n");
        }

        {
            commands::arguments set_plain{ "SET", "__cmd_test_persist_k2__", "v2" };
            CHECK(response_to_string(commands::dispatch(set_plain)) == "+OK\r\n");

            commands::arguments persist{ "PERSIST", "__cmd_test_persist_k2__" };
            CHECK(response_to_string(commands::dispatch(persist)) == ":0\r\n");
        }
    }

    TEST_CASE("hset/hget 基本行为")
    {
        commands::arguments hset_args{ "HSET", "__cmd_test_hash_1__", "name", "tom", "age", "10" };
        CHECK(response_to_string(commands::dispatch(hset_args)) == ":2\r\n");

        commands::arguments hget_name{ "HGET", "__cmd_test_hash_1__", "name" };
        CHECK(response_to_string(commands::dispatch(hget_name)) == "$3\r\ntom\r\n");

        commands::arguments hget_missing{ "HGET", "__cmd_test_hash_1__", "missing" };
        CHECK(response_to_string(commands::dispatch(hget_missing)) == "$-1\r\n");
    }

    TEST_CASE("hset 更新已有字段返回新增数量")
    {
        commands::arguments hset_init{ "HSET", "__cmd_test_hash_2__", "name", "tom" };
        CHECK(response_to_string(commands::dispatch(hset_init)) == ":1\r\n");

        commands::arguments hset_update{ "HSET", "__cmd_test_hash_2__", "name", "jerry", "city",
                                         "sh" };
        CHECK(response_to_string(commands::dispatch(hset_update)) == ":1\r\n");

        commands::arguments hget_name{ "HGET", "__cmd_test_hash_2__", "name" };
        CHECK(response_to_string(commands::dispatch(hget_name)) == "$5\r\njerry\r\n");
    }

    TEST_CASE("hgetall 返回 field-value 交替数组")
    {
        commands::arguments hset_args{ "HSET", "__cmd_test_hash_3__", "f1", "v1", "f2", "v2" };
        CHECK(response_to_string(commands::dispatch(hset_args)) == ":2\r\n");

        commands::arguments hgetall{ "HGETALL", "__cmd_test_hash_3__" };
        auto res = response_to_string(commands::dispatch(hgetall));

        CHECK(starts_with(res, "*4\r\n"));
        CHECK(res.find("$2\r\nf1\r\n") != std::string::npos);
        CHECK(res.find("$2\r\nv1\r\n") != std::string::npos);
        CHECK(res.find("$2\r\nf2\r\n") != std::string::npos);
        CHECK(res.find("$2\r\nv2\r\n") != std::string::npos);
    }

    TEST_CASE("hgetall 不存在key返回空数组")
    {
        commands::arguments hgetall{ "HGETALL", "__cmd_test_hash_missing__" };
        CHECK(response_to_string(commands::dispatch(hgetall)) == "*0\r\n");
    }

    TEST_CASE("lpush 和 lpop 顺序符合Redis语义")
    {
        commands::arguments lpush_args{ "LPUSH", "__cmd_test_list_1__", "a", "b", "c" };
        CHECK(response_to_string(commands::dispatch(lpush_args)) == ":3\r\n");

        commands::arguments lpop{ "LPOP", "__cmd_test_list_1__" };
        CHECK(response_to_string(commands::dispatch(lpop)) == "$1\r\nc\r\n");
        CHECK(response_to_string(commands::dispatch(lpop)) == "$1\r\nb\r\n");
        CHECK(response_to_string(commands::dispatch(lpop)) == "$1\r\na\r\n");
        CHECK(response_to_string(commands::dispatch(lpop)) == "$-1\r\n");
    }
}