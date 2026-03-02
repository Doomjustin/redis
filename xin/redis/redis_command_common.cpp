#include "redis_command_common.h"

#include <base_log.h>
#include <base_string_utility.h>

using namespace xin::base;

namespace xin::redis {

auto common_commands::ping(const Arguments& args) -> ResponsePtr
{
    if (args.size() == 1) {
        log::info("PING command executed with no arguments, responding with PONG");
        return std::make_unique<SimpleStringResponse>("PONG");
    }

    if (args.size() == 2) {
        log::info("PING command executed with argument: {}, responding with {}", args[1], args[1]);
        return std::make_unique<SimpleStringResponse>(args[1]);
    }

    log::error("PING command received wrong number of arguments: {}, expected 0 or 1", args);
    return std::make_unique<ErrorResponse>(arguments_size_error("ping"));
}

auto common_commands::keys(const Arguments& args) -> ResponsePtr
{
    if (args.size() < 2) {
        log::error("KEYS command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("keys"));
    }

    auto pattern = std::span{ args.begin() + 1, args.end() };
    log::info("KEYS command executed with pattern: {}", pattern);
    auto res = db().keys(pattern);

    BulkStringResponse response{};
    for (auto& item : res)
        response.add_record(std::make_shared<std::string>(item));

    log::info("KEYS command executed with pattern: {}, found {} keys", pattern, res.size());
    return std::make_unique<BulkStringResponse>(std::move(response));
}

auto common_commands::mget(const Arguments& args) -> ResponsePtr
{
    if (args.size() < 2) {
        log::error("MGET command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("mget"));
    }

    auto keys = std::span{ args.begin() + 1, args.end() };
    log::info("MGET command executed with keys: {}", keys);
    auto res = db().mget(keys);

    int found_count = 0;
    BulkStringResponse response{};
    for (auto& item : res) {
        if (item)
            ++found_count;
        response.add_record(item);
    }

    log::info("MGET command executed with keys: {}, found {} values", keys, found_count);
    return std::make_unique<BulkStringResponse>(std::move(response));
}

auto flushdb_async(const Arguments& args) -> ResponsePtr
{
    db().flush_async();
    log::info("FLUSHdb() ASYNC command executed, database clearing initiated in background");
    return std::make_unique<SimpleStringResponse>("OK");
}

auto flushdb_sync(const Arguments& args) -> ResponsePtr
{
    db().flush();
    log::info("FLUSHdb() SYNC command executed, database cleared");
    return std::make_unique<SimpleStringResponse>("OK");
}

auto common_commands::flushdb(const Arguments& args) -> ResponsePtr
{
    if (args.size() == 1)
        return flushdb_sync(args);

    if (args.size() == 2 && strings::to_lowercase(args[1]) == "async")
        return flushdb_async(args);

    if (args.size() == 2 && strings::to_lowercase(args[1]) == "sync")
        return flushdb_sync(args);

    log::error("FLUSHdb() command received wrong number of arguments or invalid option: {}", args);
    return std::make_unique<ErrorResponse>(arguments_size_error("flushdb()"));
}

auto common_commands::dbsize(const Arguments& args) -> ResponsePtr
{
    if (args.size() != 1) {
        log::error("dbsize command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("dbsize"));
    }

    auto size = db().size();
    log::info("dbsize command executed, database size: {}", size);
    return std::make_unique<IntegralResponse>(size);
}

auto common_commands::expire(const Arguments& args) -> ResponsePtr
{
    if (args.size() != 3) {
        log::error("EXPIRE command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("expire"));
    }

    auto seconds = numeric_cast<std::uint64_t>(args[2]);
    if (!seconds)
        return std::make_unique<ErrorResponse>(INVALID_INTEGRAL_ERR);

    auto key = args[1];
    bool success = db().expire_at(key, *seconds);
    log::info("EXPIRE command executed for key: {}, expire time: {} seconds, success: {}", key,
              *seconds, success);
    return std::make_unique<IntegralResponse>(success ? 1 : 0);
}

auto common_commands::ttl(const Arguments& args) -> ResponsePtr
{
    constexpr int key_not_exist = -2;
    constexpr int key_no_expire = -1;

    if (args.size() != 2) {
        log::error("TTL command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("ttl"));
    }

    auto key = args[1];
    auto res = db().ttl(key);
    if (!res) {
        if (db().contains(key)) {
            log::info("TTL command executed for key: {}, but key has no expiration", key);
            return std::make_unique<IntegralResponse>(key_no_expire);
        }

        log::info("TTL command executed for key: {}, but key does not exist", key);
        return std::make_unique<IntegralResponse>(key_not_exist);
    }

    log::info("TTL command executed for key: {}, remaining TTL: {} seconds", key, *res);
    return std::make_unique<IntegralResponse>(*res);
}

auto common_commands::persist(const Arguments& args) -> ResponsePtr
{
    if (args.size() != 2) {
        log::error("PERSIST command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("persist"));
    }

    auto key = args[1];
    if (db().persist(key)) {
        log::info("PERSIST command executed for key: {}, expiration removed", key);
        return std::make_unique<IntegralResponse>(1);
    }

    log::info("PERSIST command executed for key: {}, but key has no expiration", key);
    return std::make_unique<IntegralResponse>(0);
}

} // namespace xin::redis