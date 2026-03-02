#include "redis_command_list.h"

#include <doctest/doctest.h>
#include <redis_command_string.h>

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

TEST_SUITE("redis-command-list")
{
    using xin::redis::Arguments;
    using xin::redis::db;
    using xin::redis::list_commands;
    using xin::redis::string_commands;

    TEST_CASE("lpush and lpop order")
    {
        db().flush();

        CHECK(response_to_string(list_commands::push(
                  Arguments{ "LPUSH", "__list_k1__", "a", "b", "c" })) == ":3\r\n");

        CHECK(response_to_string(list_commands::pop(Arguments{ "LPOP", "__list_k1__" })) ==
              "$1\r\nc\r\n");
        CHECK(response_to_string(list_commands::pop(Arguments{ "LPOP", "__list_k1__" })) ==
              "$1\r\nb\r\n");
        CHECK(response_to_string(list_commands::pop(Arguments{ "LPOP", "__list_k1__" })) ==
              "$1\r\na\r\n");
        CHECK(response_to_string(list_commands::pop(Arguments{ "LPOP", "__list_k1__" })) ==
              "$-1\r\n");
    }

    TEST_CASE("lrange supports positive and negative indexes")
    {
        db().flush();
        CHECK(response_to_string(list_commands::push(
                  Arguments{ "LPUSH", "__list_k2__", "a", "b", "c" })) == ":3\r\n");

        CHECK(response_to_string(list_commands::range(Arguments{
                  "LRANGE", "__list_k2__", "0", "1" })) == "*2\r\n$1\r\nc\r\n$1\r\nb\r\n");
        CHECK(response_to_string(list_commands::range(Arguments{
                  "LRANGE", "__list_k2__", "-2", "-1" })) == "*2\r\n$1\r\nb\r\n$1\r\na\r\n");
    }

    TEST_CASE("lrange missing key returns empty list")
    {
        db().flush();
        CHECK(response_to_string(list_commands::range(
                  Arguments{ "LRANGE", "__list_missing__", "0", "2" })) == "*0\r\n");
    }

    TEST_CASE("list command argument and wrongtype errors")
    {
        CHECK(response_to_string(list_commands::push(Arguments{ "LPUSH", "k_only" })) ==
              "-ERR wrong number of Arguments for 'lpush' command\r\n");
        CHECK(response_to_string(list_commands::pop(Arguments{ "LPOP" })) ==
              "-ERR wrong number of Arguments for 'lpop' command\r\n");
        CHECK(response_to_string(list_commands::range(Arguments{ "LRANGE", "k", "0" })) ==
              "-ERR wrong number of Arguments for 'lrange' command\r\n");

        db().flush();
        CHECK(response_to_string(string_commands::set(
                  Arguments{ "SET", "__list_wrongtype_k1__", "v1" })) == "+OK\r\n");

        CHECK(
            response_to_string(list_commands::pop(Arguments{ "LPOP", "__list_wrongtype_k1__" })) ==
            "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n");
        CHECK(response_to_string(
                  list_commands::range(Arguments{ "LRANGE", "__list_wrongtype_k1__", "0", "1" })) ==
              "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n");
    }
}