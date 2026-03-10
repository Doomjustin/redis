#include "redis_command_list.h"

#include <doctest/doctest.h>
#include <redis_application_context.h>
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

std::size_t db_index = 0;

} // namespace

TEST_SUITE("redis-command-list")
{
    using xin::redis::application_context;
    using xin::redis::Arguments;
    using xin::redis::list_commands;
    using xin::redis::string_commands;

    TEST_CASE("lpush and lpop order")
    {
        application_context::db(db_index).flush();

        CHECK(response_to_string(list_commands::push(
                  application_context::db(db_index), Arguments{ "LPUSH", "__list_k1__", "a", "b", "c" })) == ":3\r\n");

        CHECK(response_to_string(list_commands::pop(
                  application_context::db(db_index), Arguments{ "LPOP", "__list_k1__" })) == "$1\r\nc\r\n");
        CHECK(response_to_string(list_commands::pop(
                  application_context::db(db_index), Arguments{ "LPOP", "__list_k1__" })) == "$1\r\nb\r\n");
        CHECK(response_to_string(list_commands::pop(
                  application_context::db(db_index), Arguments{ "LPOP", "__list_k1__" })) == "$1\r\na\r\n");
        CHECK(response_to_string(
                  list_commands::pop(application_context::db(db_index), Arguments{ "LPOP", "__list_k1__" })) == "$-1\r\n");
    }

    TEST_CASE("lrange supports positive and negative indexes")
    {
        application_context::db(db_index).flush();
        CHECK(response_to_string(list_commands::push(
                  application_context::db(db_index), Arguments{ "LPUSH", "__list_k2__", "a", "b", "c" })) == ":3\r\n");

        CHECK(response_to_string(
                  list_commands::range(application_context::db(db_index), Arguments{ "LRANGE", "__list_k2__", "0", "1" })) ==
              "*2\r\n$1\r\nc\r\n$1\r\nb\r\n");
        CHECK(response_to_string(list_commands::range(
                  application_context::db(db_index), Arguments{ "LRANGE", "__list_k2__", "-2", "-1" })) ==
              "*2\r\n$1\r\nb\r\n$1\r\na\r\n");
    }

    TEST_CASE("lrange missing key returns empty list")
    {
        application_context::db(db_index).flush();
        CHECK(response_to_string(list_commands::range(
                  application_context::db(db_index), Arguments{ "LRANGE", "__list_missing__", "0", "2" })) == "*0\r\n");
    }

    TEST_CASE("list command argument and wrongtype errors")
    {
        CHECK(response_to_string(list_commands::push(application_context::db(db_index), Arguments{ "LPUSH", "k_only" })) ==
              "-ERR wrong number of arguments for 'lpush' command\r\n");
        CHECK(response_to_string(list_commands::pop(application_context::db(db_index), Arguments{ "LPOP" })) ==
              "-ERR wrong number of arguments for 'lpop' command\r\n");
        CHECK(response_to_string(list_commands::range(application_context::db(db_index), Arguments{ "LRANGE", "k", "0" })) ==
              "-ERR wrong number of arguments for 'lrange' command\r\n");

        application_context::db(db_index).flush();
        CHECK(response_to_string(string_commands::set(
                  application_context::db(db_index), Arguments{ "SET", "__list_wrongtype_k1__", "v1" })) == "+OK\r\n");

        CHECK(response_to_string(
                  list_commands::pop(application_context::db(db_index), Arguments{ "LPOP", "__list_wrongtype_k1__" })) ==
              "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n");
        CHECK(response_to_string(list_commands::range(
                  application_context::db(db_index), Arguments{ "LRANGE", "__list_wrongtype_k1__", "0", "1" })) ==
              "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n");
    }
}