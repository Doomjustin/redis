#include "redis_command_common.h"

#include <base_log.h>
#include <base_string_utility.h>
#include <redis_application_context.h>

using namespace xin::base;

namespace xin::redis {

auto common_commands::select(std::size_t& index, const Arguments& args) -> ResponsePtr
{
    if (args.size() != 2) {
        log::info("SELECT command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("select"));
    }

    auto new_index = numeric_cast<std::size_t>(args[1]);
    if (!new_index)
        return std::make_unique<ErrorResponse>(INVALID_INTEGRAL_ERR);

    if (*new_index >= application_context::DB_COUNT)
        return std::make_unique<ErrorResponse>("ERR invalid DB index");

    index = *new_index;
    log::debug("SELECT command executed, selected database: {}", *new_index);

    return std::make_unique<SimpleStringResponse>("OK");
}

auto common_commands::ping(Database& db, const Arguments& args) -> ResponsePtr
{
    if (args.size() == 1) {
        log::debug("PING command executed with no arguments, responding with PONG");
        return std::make_unique<SimpleStringResponse>("PONG");
    }

    if (args.size() == 2) {
        log::debug("PING command executed with argument: {}, responding with {}", args[1], args[1]);
        return std::make_unique<SimpleStringResponse>(args[1]);
    }

    log::info("PING command received wrong number of arguments: {}, expected 0 or 1", args);
    return std::make_unique<ErrorResponse>(arguments_size_error("ping"));
}

auto common_commands::keys(Database& db, const Arguments& args) -> ResponsePtr
{
    if (args.size() < 2) {
        log::info("KEYS command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("keys"));
    }

    auto pattern = std::span{ args.begin() + 1, args.end() };
    log::debug("KEYS command executed with pattern: {}", pattern);
    auto res = db.keys(pattern);

    ArrayResponse response{};
    for (auto& item : res)
        response.add_record(std::make_shared<std::string>(item));

    log::debug("KEYS command executed with pattern: {}, found {} keys", pattern, res.size());
    return std::make_unique<ArrayResponse>(std::move(response));
}

auto common_commands::mget(Database& db, const Arguments& args) -> ResponsePtr
{
    if (args.size() < 2) {
        log::info("MGET command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("mget"));
    }

    auto keys = std::span{ args.begin() + 1, args.end() };
    log::debug("MGET command executed with keys: {}", keys);
    auto res = db.mget(keys);

    int found_count = 0;
    ArrayResponse response{};
    for (auto& item : res) {
        if (item)
            ++found_count;
        response.add_record(item);
    }

    log::debug("MGET command executed with keys: {}, found {} values", keys, found_count);
    return std::make_unique<ArrayResponse>(std::move(response));
}

auto common_commands::del(Database& db, const Arguments& args) -> ResponsePtr
{
    if (args.size() < 2) {
        log::info("DEL command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("del"));
    }

    auto keys = std::span{ args.begin() + 1, args.end() };
    auto erased_count = db.erase(keys);

    log::debug("DEL command executed with keys: {}, erased {} keys", keys, erased_count);
    return std::make_unique<IntegralResponse>(erased_count);
}

auto flushdb_async(Database& db, const Arguments& args) -> ResponsePtr
{
    db.flush_async();
    log::debug("FLUSHDB ASYNC command executed, database clearing initiated "
               "in background");
    return std::make_unique<SimpleStringResponse>("OK");
}

auto flushdb_sync(Database& db, const Arguments& args) -> ResponsePtr
{
    db.flush();
    log::debug("FLUSHDB SYNC command executed, database cleared");
    return std::make_unique<SimpleStringResponse>("OK");
}

auto common_commands::flushdb(Database& db, const Arguments& args) -> ResponsePtr
{
    if (args.size() == 1)
        return flushdb_sync(db, args);

    if (args.size() == 2 && strings::to_lowercase(args[1]) == "async")
        return flushdb_async(db, args);

    if (args.size() == 2 && strings::to_lowercase(args[1]) == "sync")
        return flushdb_sync(db, args);

    log::info("FLUSHDB command received wrong number of arguments or "
              "invalid option: {}",
              args);
    return std::make_unique<ErrorResponse>(arguments_size_error("flushdb"));
}

auto common_commands::dbsize(Database& db, const Arguments& args) -> ResponsePtr
{
    if (args.size() != 1) {
        log::info("dbsize command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("dbsize"));
    }

    auto size = db.size();
    log::debug("dbsize command executed, database size: {}", size);
    return std::make_unique<IntegralResponse>(size);
}

auto common_commands::expire(Database& db, const Arguments& args) -> ResponsePtr
{
    if (args.size() != 3) {
        log::info("EXPIRE command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("expire"));
    }

    auto seconds = numeric_cast<std::uint64_t>(args[2]);
    if (!seconds)
        return std::make_unique<ErrorResponse>(INVALID_INTEGRAL_ERR);

    auto key = args[1];
    bool success = db.expire_at(key, *seconds);
    log::debug("EXPIRE command executed for key: {}, expire time: {} seconds, success: {}", key,
               *seconds, success);

    return std::make_unique<IntegralResponse>(success ? 1 : 0);
}

auto common_commands::ttl(Database& db, const Arguments& args) -> ResponsePtr
{
    constexpr int key_not_exist = -2;
    constexpr int key_no_expire = -1;

    if (args.size() != 2) {
        log::info("TTL command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("ttl"));
    }

    auto key = args[1];
    auto res = db.ttl(key);
    if (!res) {
        if (db.contains(key)) {
            log::debug("TTL command executed for key: {}, but key has no expiration", key);
            return std::make_unique<IntegralResponse>(key_no_expire);
        }

        log::debug("TTL command executed for key: {}, but key does not exist", key);
        return std::make_unique<IntegralResponse>(key_not_exist);
    }

    log::debug("TTL command executed for key: {}, remaining TTL: {} seconds", key, *res);
    return std::make_unique<IntegralResponse>(*res);
}

auto common_commands::persist(Database& db, const Arguments& args) -> ResponsePtr
{
    if (args.size() != 2) {
        log::info("PERSIST command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("persist"));
    }

    auto key = args[1];

    if (db.persist(key)) {
        log::debug("PERSIST command executed for key: {}, expiration removed", key);
        return std::make_unique<IntegralResponse>(1);
    }

    log::debug("PERSIST command executed for key: {}, but key has no expiration", key);
    return std::make_unique<IntegralResponse>(0);
}

auto common_commands::exists(Database& db, const Arguments& args) -> ResponsePtr
{
    if (args.size() < 2) {
        log::info("EXISTS command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("exists"));
    }

    std::size_t count = 0;
    for (std::size_t i = 1; i < args.size(); ++i) {
        if (db.contains(args[i]))
            ++count;
    }

    log::debug("EXISTS command executed, number of existing keys: {}", count);
    return std::make_unique<IntegralResponse>(count);
}

} // namespace xin::redis