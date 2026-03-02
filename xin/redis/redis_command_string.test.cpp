#include "redis_command_string.h"

#include "redis_command_list.h"

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

TEST_SUITE("redis-command-string")
{
    using xin::redis::arguments;
    using xin::redis::db;
    using xin::redis::list_commands;
    using xin::redis::string_commands;

    TEST_CASE("set and get basic path")
    {
        db().flush();

        CHECK(response_to_string(string_commands::set(arguments{ "SET", "__string_k1__", "v1" })) ==
              "+OK\r\n");
        CHECK(response_to_string(string_commands::get(arguments{ "GET", "__string_k1__" })) ==
              "$2\r\nv1\r\n");
    }

    TEST_CASE("get missing key returns null bulk")
    {
        db().flush();
        CHECK(response_to_string(string_commands::get(arguments{ "GET", "__string_missing__" })) ==
              "$-1\r\n");
    }

    TEST_CASE("set with ex option")
    {
        db().flush();
        CHECK(response_to_string(string_commands::set(
                  arguments{ "SET", "__string_ex_k1__", "v1", "EX", "60" })) == "+OK\r\n");
    }

    TEST_CASE("set and get argument validation")
    {
        CHECK(response_to_string(string_commands::set(arguments{ "SET", "k_only" })) ==
              "-ERR wrong number of arguments for 'set' command\r\n");
        CHECK(response_to_string(string_commands::get(arguments{ "GET" })) ==
              "-ERR wrong number of arguments for 'get' command\r\n");
        CHECK(response_to_string(
                  string_commands::set(arguments{ "SET", "k", "v", "EX", "not_int" })) ==
              "-ERR value is not an integer or out of range\r\n");
    }

    TEST_CASE("get returns wrongtype when key is list")
    {
        db().flush();
        CHECK(response_to_string(list_commands::push(
                  arguments{ "LPUSH", "__string_wrongtype_k1__", "a" })) == ":1\r\n");

        CHECK(response_to_string(
                  string_commands::get(arguments{ "GET", "__string_wrongtype_k1__" })) ==
              "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n");
    }
}