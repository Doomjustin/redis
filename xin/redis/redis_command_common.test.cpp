#include "redis_command_common.h"

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

void reset_all_databases()
{
    using xin::redis::application_context;
    for (auto& db : application_context::databases)
        db.flush();
}

std::size_t db_index = 0;

} // namespace

TEST_SUITE("redis-command-common")
{
    using xin::redis::application_context;
    using xin::redis::Arguments;
    using xin::redis::common_commands;
    using xin::redis::string_commands;

    TEST_CASE("ping returns pong or echo")
    {
        CHECK(response_to_string(common_commands::ping(application_context::db(db_index), Arguments{ "PING" })) ==
              "+PONG\r\n");
        CHECK(response_to_string(common_commands::ping(application_context::db(db_index), Arguments{ "PING", "hello" })) ==
              "+hello\r\n");
    }

    TEST_CASE("ping with invalid arity returns error")
    {
        CHECK(response_to_string(common_commands::ping(application_context::db(db_index), Arguments{ "PING", "a", "b" })) ==
              "-ERR wrong number of arguments for 'ping' command\r\n");
    }

    TEST_CASE("mget returns values and nil for missing keys")
    {
        reset_all_databases();
        CHECK(response_to_string(string_commands::set(
                  application_context::db(db_index), Arguments{ "SET", "__common_mget_k1__", "v1" })) == "+OK\r\n");

        const auto resp = common_commands::mget(
            application_context::db(db_index), Arguments{ "MGET", "__common_mget_k1__", "__common_mget_missing__" });
        CHECK(response_to_string(resp) == "*2\r\n$2\r\nv1\r\n$-1\r\n");
    }

    TEST_CASE("dbsize and flushdb work")
    {
        reset_all_databases();
        CHECK(response_to_string(common_commands::dbsize(application_context::db(db_index), Arguments{ "DBSIZE" })) ==
              ":0\r\n");

        CHECK(response_to_string(string_commands::set(
                  application_context::db(db_index), Arguments{ "SET", "__common_size_k1__", "v1" })) == "+OK\r\n");
        CHECK(response_to_string(common_commands::dbsize(application_context::db(db_index), Arguments{ "DBSIZE" })) ==
              ":1\r\n");

        CHECK(response_to_string(
                  common_commands::flushdb(application_context::db(db_index), Arguments{ "FLUSHDB", "SYNC" })) == "+OK\r\n");
        CHECK(response_to_string(common_commands::dbsize(application_context::db(db_index), Arguments{ "DBSIZE" })) ==
              ":0\r\n");
    }

    TEST_CASE("flushdb invalid option returns arity error")
    {
        reset_all_databases();

        CHECK(response_to_string(
                  common_commands::flushdb(application_context::db(db_index), Arguments{ "FLUSHDB", "MAYBE" })) ==
              "-ERR wrong number of arguments for 'flushdb' command\r\n");
    }

    TEST_CASE("expire ttl persist basic path")
    {
        reset_all_databases();
        CHECK(response_to_string(string_commands::set(
                  application_context::db(db_index), Arguments{ "SET", "__common_ttl_k1__", "v1" })) == "+OK\r\n");

        CHECK(response_to_string(common_commands::expire(
                  application_context::db(db_index), Arguments{ "EXPIRE", "__common_ttl_k1__", "60" })) == ":1\r\n");

        const auto ttl_resp = response_to_string(
            common_commands::ttl(application_context::db(db_index), Arguments{ "TTL", "__common_ttl_k1__" }));
        CHECK(ttl_resp[0] == ':');

        CHECK(response_to_string(common_commands::persist(
                  application_context::db(db_index), Arguments{ "PERSIST", "__common_ttl_k1__" })) == ":1\r\n");
        CHECK(response_to_string(common_commands::ttl(
                  application_context::db(db_index), Arguments{ "TTL", "__common_ttl_k1__" })) == ":-1\r\n");
    }

    TEST_CASE("del removes existing keys and ignores missing keys")
    {
        reset_all_databases();
        CHECK(response_to_string(string_commands::set(
                  application_context::db(db_index), Arguments{ "SET", "__del_k1__", "v1" })) == "+OK\r\n");
        CHECK(response_to_string(string_commands::set(
                  application_context::db(db_index), Arguments{ "SET", "__del_k2__", "v2" })) == "+OK\r\n");

        CHECK(response_to_string(common_commands::del(
                  application_context::db(db_index), Arguments{ "DEL", "__del_k1__", "__del_missing__", "__del_k2__" })) ==
              ":2\r\n");
        CHECK(response_to_string(common_commands::exists(
                  application_context::db(db_index), Arguments{ "EXISTS", "__del_k1__", "__del_k2__" })) == ":0\r\n");
    }

    TEST_CASE("del with single key returns erased count")
    {
        reset_all_databases();
        CHECK(response_to_string(string_commands::set(
                  application_context::db(db_index), Arguments{ "SET", "__del_single_k1__", "v1" })) == "+OK\r\n");

        CHECK(response_to_string(common_commands::del(
                  application_context::db(db_index), Arguments{ "DEL", "__del_single_k1__" })) == ":1\r\n");
        CHECK(response_to_string(common_commands::exists(
                  application_context::db(db_index), Arguments{ "EXISTS", "__del_single_k1__" })) == ":0\r\n");
    }

    TEST_CASE("exists returns number of existing keys")
    {
        reset_all_databases();
        CHECK(response_to_string(string_commands::set(
                  application_context::db(db_index), Arguments{ "SET", "__exists_k1__", "v1" })) == "+OK\r\n");

        CHECK(response_to_string(common_commands::exists(
                  application_context::db(db_index), Arguments{ "EXISTS", "__exists_k1__", "__exists_k1__",
                                       "__exists_missing__" })) == ":2\r\n");
    }
}