#include "redis_command.h"

#include "redis_response.h"
#include "redis_storage.h"

#include <base_log.h>
#include <base_string_utility.h>
#include <fmt/core.h>

#include <charconv>
#include <concepts>
#include <string>
#include <unordered_map>

namespace {

using namespace xin::redis;
using namespace xin::base;
using arguments = commands::arguments;
using response = commands::response;
using handler = std::function<response(const arguments&)>;

xin::redis::Database db{};

template<std::integral T>
auto numeric_cast(std::string_view str) -> std::optional<T>
{
    T seconds;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), seconds);
    if (ec != std::errc()) {
        log::error("EXPIRE command received invalid expiration time: {}", str);
        return {};
    }

    return seconds;
}

auto set_with_expiry(const arguments& args) -> response
{
    auto expiry = numeric_cast<std::uint64_t>(args[4]);
    if (!expiry)
        return std::make_unique<ErrorResponse>("ERR value is not an integer or out of range");

    db.set(args[1], std::make_shared<std::string>(args[2]), *expiry);
    log::info("SET command with expiry executed with key: {}, value: {}, expire time: {} seconds",
              args[1], args[2], *expiry);
    return std::make_unique<SimpleStringResponse>("OK");
}

auto set_impl(const arguments& args) -> response
{
    db.set(args[1], std::make_shared<std::string>(args[2]));
    log::info("SET command executed with key: {} and value: {}", args[1], args[2]);
    return std::make_unique<SimpleStringResponse>("OK");
}

// set key value [EX seconds]
auto set(const arguments& args) -> response
{
    if (args.size() == 3)
        return set_impl(args);

    if (args.size() == 5 && strings::to_lowercase(args[3]) == "ex")
        return set_with_expiry(args);

    log::error("SET command received wrong number of arguments or invalid option: {}", args);
    return std::make_unique<ErrorResponse>("ERR wrong number of arguments for 'set' command");
}

auto get(const arguments& args) -> response
{
    if (args.size() != 2) {
        log::error("GET command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>("ERR wrong number of arguments for 'get' command");
    }

    auto res = db.get(args[1]);
    if (!res) {
        log::info("GET command executed with key: {}, but key does not exist", args[1]);
        return std::make_unique<NullBulkStringResponse>();
    }

    SingleBulkStringResponse response{ *res };
    log::info("GET command executed with key: {} and value: {}", args[1], **res);
    return std::make_unique<SingleBulkStringResponse>(std::move(response));
}

auto ping(const arguments& args) -> response
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
    return std::make_unique<ErrorResponse>("ERR wrong number of arguments for 'ping' command");
}

auto keys(const arguments& args) -> response
{
    if (args.size() < 2) {
        log::error("KEYS command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>("ERR wrong number of arguments for 'keys' command");
    }

    auto pattern = std::span{ args.begin() + 1, args.end() };
    log::info("KEYS command executed with pattern: {}", pattern);
    auto res = db.get(pattern);

    int found_count = 0;
    BulkStringResponse response{};
    for (auto& item : res) {
        if (item) {
            response.add_record(item);
            ++found_count;
        }
    }

    log::info("KEYS command executed with pattern: {}, found {} keys", pattern, found_count);
    return std::make_unique<BulkStringResponse>(std::move(response));
}

auto mget(const arguments& args) -> response
{
    if (args.size() < 2) {
        log::error("MGET command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>("ERR wrong number of arguments for 'mget' command");
    }

    auto keys = std::span{ args.begin() + 1, args.end() };
    log::info("MGET command executed with keys: {}", keys);
    auto res = db.get(keys);

    int found_count = 0;
    BulkStringResponse response{};
    for (auto& item : res) {
        if (item) {
            ++found_count;
            response.add_record(item);
        }
        else {
            response.add_record(std::make_shared<std::string>(""));
        }
    }

    log::info("MGET command executed with keys: {}, found {} values", keys, found_count);
    return std::make_unique<BulkStringResponse>(std::move(response));
}

auto flushdb_async(const arguments& args) -> response
{
    db.flush_async();
    log::info("FLUSHDB ASYNC command executed, database clearing initiated in background");
    return std::make_unique<SimpleStringResponse>("OK");
}

auto flushdb_sync(const arguments& args) -> response
{
    db.flush();
    log::info("FLUSHDB SYNC command executed, database cleared");
    return std::make_unique<SimpleStringResponse>("OK");
}

auto flushdb(const arguments& args) -> response
{
    if (args.size() == 1)
        return flushdb_sync(args);

    if (args.size() == 2 && strings::to_lowercase(args[1]) == "async")
        return flushdb_async(args);

    if (args.size() == 2 && strings::to_lowercase(args[1]) == "sync")
        return flushdb_sync(args);

    log::error("FLUSHDB command received wrong number of arguments or invalid option: {}", args);
    return std::make_unique<ErrorResponse>("ERR wrong number of arguments for 'flushdb' command");
}

auto dbsize(const arguments& args) -> response
{
    if (args.size() != 1) {
        log::error("DBSIZE command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(
            "ERR wrong number of arguments for 'dbsize' command");
    }

    auto size = db.size();
    log::info("DBSIZE command executed, database size: {}", size);
    return std::make_unique<IntegralResponse>(size);
}

auto expire(const arguments& args) -> response
{
    if (args.size() != 3) {
        log::error("EXPIRE command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(
            "ERR wrong number of arguments for 'expire' command");
    }

    auto seconds = numeric_cast<std::uint64_t>(args[2]);
    if (!seconds)
        return std::make_unique<ErrorResponse>("ERR value is not an integer or out of range");

    auto key = args[1];
    bool success = db.expire_at(key, *seconds);
    log::info("EXPIRE command executed for key: {}, expire time: {} seconds, success: {}", key,
              *seconds, success);
    return std::make_unique<IntegralResponse>(success ? 1 : 0);
}

auto ttl(const arguments& args) -> response
{
    constexpr int key_not_exist = -2;
    constexpr int key_no_expire = -1;

    if (args.size() != 2) {
        log::error("TTL command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>("ERR wrong number of arguments for 'ttl' command");
    }

    auto key = args[1];
    auto res = db.ttl(key);
    if (!res) {
        if (db.contains(key)) {
            log::info("TTL command executed for key: {}, but key has no expiration", key);
            return std::make_unique<IntegralResponse>(key_no_expire);
        }

        log::info("TTL command executed for key: {}, but key does not exist", key);
        return std::make_unique<IntegralResponse>(key_not_exist);
    }

    log::info("TTL command executed for key: {}, remaining TTL: {} seconds", key, *res);
    return std::make_unique<IntegralResponse>(*res);
}

auto persist(const arguments& args) -> response
{
    if (args.size() != 2) {
        log::error("PERSIST command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(
            "ERR wrong number of arguments for 'persist' command");
    }

    auto key = args[1];
    db.persist(key);
    log::info("PERSIST command executed for key: {}", key);
    return std::make_unique<IntegralResponse>(1);
}

using handlers_table = std::unordered_map<std::string, handler>;
handlers_table handlers = { { "set", set },        { "get", get },       { "ping", ping },
                            { "keys", keys },      { "mget", mget },     { "flushdb", flushdb },
                            { "dbsize", dbsize },  { "expire", expire }, { "ttl", ttl },
                            { "persist", persist } };

} // namespace

namespace xin::redis {

auto commands::dispatch(const arguments& args) -> response
{
    if (args.empty())
        return std::make_unique<ErrorResponse>("ERR empty command");

    using namespace xin::base;
    auto it = handlers.find(strings::to_lowercase(args[0]));
    if (it == handlers.end())
        return std::make_unique<ErrorResponse>(fmt::format("ERR unknown command '{}'", args[0]));

    return it->second(args);
}

} // namespace xin::redis