#include "redis_command_sorted_set.h"

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

TEST_SUITE("redis-command-sorted-set")
{
    using xin::redis::Arguments;
    using xin::redis::db;
    using xin::redis::sorted_set_commands;
    using xin::redis::string_commands;

    TEST_CASE("zadd and zrange basic path")
    {
        db().flush();

        CHECK(response_to_string(sorted_set_commands::add(
                  Arguments{ "ZADD", "__zset_k1__", "1", "one", "2", "two" })) == ":2\r\n");

        CHECK(response_to_string(sorted_set_commands::range(Arguments{
                  "ZRANGE", "__zset_k1__", "0", "-1" })) == "*2\r\n$3\r\none\r\n$3\r\ntwo\r\n");
    }

    TEST_CASE("zrange withscores returns member-score pairs")
    {
        db().flush();

        CHECK(response_to_string(sorted_set_commands::add(
                  Arguments{ "ZADD", "__zset_k2__", "1", "one", "2", "two" })) == ":2\r\n");

        CHECK(response_to_string(sorted_set_commands::range(
                  Arguments{ "ZRANGE", "__zset_k2__", "0", "-1", "WITHSCORES" })) ==
              "*4\r\n$3\r\none\r\n$1\r\n1\r\n$3\r\ntwo\r\n$1\r\n2\r\n");
    }

    TEST_CASE("zrange supports negative indexes")
    {
        db().flush();

        CHECK(response_to_string(sorted_set_commands::add(Arguments{
                  "ZADD", "__zset_k3__", "1", "one", "2", "two", "3", "three" })) == ":3\r\n");

        CHECK(response_to_string(sorted_set_commands::range(Arguments{
                  "ZRANGE", "__zset_k3__", "-2", "-1" })) == "*2\r\n$3\r\ntwo\r\n$5\r\nthree\r\n");
    }

    TEST_CASE("zadd updates existing member score and not count as new")
    {
        db().flush();

        CHECK(response_to_string(sorted_set_commands::add(
                  Arguments{ "ZADD", "__zset_k4__", "1", "one" })) == ":1\r\n");
        CHECK(response_to_string(sorted_set_commands::add(
                  Arguments{ "ZADD", "__zset_k4__", "2", "one" })) == ":0\r\n");

        CHECK(response_to_string(sorted_set_commands::range(
                  Arguments{ "ZRANGE", "__zset_k4__", "0", "-1", "WITHSCORES" })) ==
              "*2\r\n$3\r\none\r\n$1\r\n2\r\n");
    }

    TEST_CASE("zset argument and wrongtype errors")
    {
        CHECK(response_to_string(sorted_set_commands::add(Arguments{ "ZADD", "k", "1" })) ==
              "-ERR wrong number of Arguments for 'zadd' command\r\n");

        CHECK(response_to_string(sorted_set_commands::add(Arguments{
                  "ZADD", "k", "bad", "one" })) == "-ERR value is not a valid float\r\n");

        CHECK(response_to_string(sorted_set_commands::range(Arguments{ "ZRANGE", "k", "0" })) ==
              "-ERR wrong number of Arguments for 'zrange' command\r\n");

        CHECK(response_to_string(sorted_set_commands::range(
                  Arguments{ "ZRANGE", "k", "0", "-1", "BADSCORES" })) == "-ERR syntax error\r\n");

        CHECK(response_to_string(
                  sorted_set_commands::range(Arguments{ "ZRANGE", "k", "bad", "-1" })) ==
              "-ERR value is not an integer or out of range\r\n");

        db().flush();
        CHECK(response_to_string(string_commands::set(
                  Arguments{ "SET", "__zset_wrongtype_k1__", "v1" })) == "+OK\r\n");

        CHECK(response_to_string(sorted_set_commands::add(
                  Arguments{ "ZADD", "__zset_wrongtype_k1__", "1", "one" })) ==
              "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n");
        CHECK(response_to_string(sorted_set_commands::range(
                  Arguments{ "ZRANGE", "__zset_wrongtype_k1__", "0", "-1" })) ==
              "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n");
    }
}