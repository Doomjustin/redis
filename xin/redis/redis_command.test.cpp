#include "redis_command.h"

#include <doctest/doctest.h>
#include <redis_application_context.h>

#include <string>

namespace {

auto response_to_string(const xin::redis::ResponsePtr& resp) -> std::string
{
    std::string out;
    for (const auto& buffer : resp->to_buffer())
        out.append(static_cast<const char*>(buffer.data()), buffer.size());

    return out;
}

std::size_t db_index = 0;

void reset_all_databases()
{
    using xin::redis::application_context;
    db_index = 0;
    for (auto& db : application_context::databases)
        db.flush();
}

} // namespace

TEST_SUITE("redis-command")
{
    using xin::redis::application_context;
    using xin::redis::Arguments;
    using xin::redis::commands;

    TEST_CASE("dispatch 空命令返回错误")
    {
        Arguments args{};
        const auto resp = commands::dispatch(db_index, args);

        CHECK(response_to_string(resp) == "-ERR empty command\r\n");
    }

    TEST_CASE("dispatch 未知命令返回错误")
    {
        Arguments args{ "NOPE" };
        const auto resp = commands::dispatch(db_index, args);

        CHECK(response_to_string(resp) == "-ERR unknown command 'NOPE'\r\n");
    }

    TEST_CASE("dispatch 命令名大小写不敏感")
    {
        reset_all_databases();

        CHECK(response_to_string(commands::dispatch(db_index, Arguments{ "ping" })) == "+PONG\r\n");
        CHECK(response_to_string(commands::dispatch(db_index, Arguments{ "PiNg", "echo" })) ==
              "+echo\r\n");
    }

    TEST_CASE("dispatch routes to modules")
    {
        reset_all_databases();

        CHECK(response_to_string(commands::dispatch(
                  db_index, Arguments{ "set", "__dispatch_k1__", "v1" })) == "+OK\r\n");
        CHECK(response_to_string(commands::dispatch(
                  db_index, Arguments{ "get", "__dispatch_k1__" })) == "$2\r\nv1\r\n");

        CHECK(response_to_string(commands::dispatch(
                  db_index, Arguments{ "hset", "__dispatch_h1__", "name", "tom" })) == ":1\r\n");
        CHECK(response_to_string(commands::dispatch(
                  db_index, Arguments{ "hget", "__dispatch_h1__", "name" })) == "$3\r\ntom\r\n");

        CHECK(response_to_string(commands::dispatch(
                  db_index, Arguments{ "lpush", "__dispatch_l1__", "a", "b" })) == ":2\r\n");
        CHECK(response_to_string(commands::dispatch(
                  db_index, Arguments{ "lrange", "__dispatch_l1__", "0", "-1" })) ==
              "*2\r\n$1\r\nb\r\n$1\r\na\r\n");
    }

    TEST_CASE("dispatch select changes active database index")
    {
        reset_all_databases();

        CHECK(response_to_string(commands::dispatch(
                  db_index, Arguments{ "set", "__dispatch_select_k__", "db0" })) == "+OK\r\n");
        CHECK(response_to_string(commands::dispatch(db_index, Arguments{ "SELECT", "1" })) ==
              "+OK\r\n");
        CHECK(db_index == 1);

        CHECK(response_to_string(commands::dispatch(db_index, Arguments{ "dbsize" })) == ":0\r\n");
        CHECK(response_to_string(commands::dispatch(
                  db_index, Arguments{ "set", "__dispatch_select_k__", "db1" })) == "+OK\r\n");

        CHECK(response_to_string(commands::dispatch(db_index, Arguments{ "select", "0" })) ==
              "+OK\r\n");
        CHECK(db_index == 0);
        CHECK(response_to_string(commands::dispatch(
                  db_index, Arguments{ "get", "__dispatch_select_k__" })) == "$3\r\ndb0\r\n");
    }

    TEST_CASE("dispatch select invalid input does not change index")
    {
        reset_all_databases();
        db_index = 3;

        CHECK(response_to_string(commands::dispatch(db_index, Arguments{ "SELECT" })) ==
              "-ERR wrong number of arguments for 'select' command\r\n");
        CHECK(db_index == 3);

        CHECK(response_to_string(commands::dispatch(db_index, Arguments{ "SELECT", "-1" })) ==
              "-ERR value is not an integer or out of range\r\n");
        CHECK(db_index == 3);

        CHECK(response_to_string(commands::dispatch(db_index, Arguments{ "SELECT", "16" })) ==
              "-ERR invalid DB index\r\n");
        CHECK(db_index == 3);
    }
}