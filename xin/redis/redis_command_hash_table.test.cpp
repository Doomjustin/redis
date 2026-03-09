#include "redis_command_hash_table.h"

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

auto starts_with(const std::string& value, const std::string& prefix) -> bool
{
    return value.rfind(prefix, 0) == 0;
}

std::size_t db_index = 0;

} // namespace

TEST_SUITE("redis-command-hash")
{
    using xin::redis::application_context;
    using xin::redis::Arguments;
    using xin::redis::hash_table_commands;
    using xin::redis::string_commands;

    TEST_CASE("hset and hget basic behavior")
    {
        application_context::db(db_index).flush();

        CHECK(response_to_string(hash_table_commands::set(
                  db_index, Arguments{ "HSET", "__hash_k1__", "name", "tom", "age", "10" })) ==
              ":2\r\n");
        CHECK(response_to_string(hash_table_commands::get(
                  db_index, Arguments{ "HGET", "__hash_k1__", "name" })) == "$3\r\ntom\r\n");
        CHECK(response_to_string(hash_table_commands::get(
                  db_index, Arguments{ "HGET", "__hash_k1__", "missing" })) == "$-1\r\n");
    }

    TEST_CASE("hset update returns number of newly added fields")
    {
        application_context::db(db_index).flush();
        CHECK(response_to_string(hash_table_commands::set(
                  db_index, Arguments{ "HSET", "__hash_k2__", "name", "tom" })) == ":1\r\n");

        CHECK(response_to_string(hash_table_commands::set(
                  db_index, Arguments{ "HSET", "__hash_k2__", "name", "jerry", "city", "sh" })) ==
              ":1\r\n");
    }

    TEST_CASE("hgetall returns alternating field-value array")
    {
        application_context::db(db_index).flush();
        CHECK(response_to_string(hash_table_commands::set(
                  db_index, Arguments{ "HSET", "__hash_k3__", "f1", "v1", "f2", "v2" })) ==
              ":2\r\n");

        auto res = response_to_string(
            hash_table_commands::get_all(db_index, Arguments{ "HGETALL", "__hash_k3__" }));
        CHECK(starts_with(res, "*4\r\n"));
        CHECK(res.find("$2\r\nf1\r\n") != std::string::npos);
        CHECK(res.find("$2\r\nv1\r\n") != std::string::npos);
        CHECK(res.find("$2\r\nf2\r\n") != std::string::npos);
        CHECK(res.find("$2\r\nv2\r\n") != std::string::npos);
    }

    TEST_CASE("hash argument and wrongtype errors")
    {
        CHECK(
            response_to_string(hash_table_commands::set(db_index, Arguments{ "HSET", "k", "f" })) ==
            "-ERR wrong number of arguments for 'hset' command\r\n");
        CHECK(response_to_string(hash_table_commands::get(db_index, Arguments{ "HGET", "k" })) ==
              "-ERR wrong number of arguments for 'hget' command\r\n");
        CHECK(response_to_string(
                  hash_table_commands::get_all(db_index, Arguments{ "HGETALL", "k", "x" })) ==
              "-ERR wrong number of arguments for 'hgetall' command\r\n");

        application_context::db(db_index).flush();
        CHECK(response_to_string(string_commands::set(
                  db_index, Arguments{ "SET", "__hash_wrongtype_k1__", "v1" })) == "+OK\r\n");
        CHECK(response_to_string(hash_table_commands::get(
                  db_index, Arguments{ "HGET", "__hash_wrongtype_k1__", "f" })) ==
              "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n");
    }
}