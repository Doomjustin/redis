#include "redis_command.h"

#include "redis_response.h"
#include "redis_storage.h"

#include <base_formats.h>
#include <base_log.h>
#include <base_string_utility.h>
#include <fmt/core.h>

#include <charconv>
#include <concepts>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace {

using namespace xin::redis;
using namespace xin::base;
using arguments = commands::arguments;
using response = commands::response;
using handler = std::function<response(const arguments&)>;
using string_type = Database::string_type;
using hash_type = Database::hash_type;
using list_type = Database::list_type;

xin::redis::Database db{};

constexpr std::string_view WRONG_TYPE_ERR =
    "WRONGTYPE Operation against a key holding the wrong kind of value";

constexpr std::string_view INVALID_EXPIRY_ERR = "ERR value is not an integer or out of range";

auto arguments_size_error(std::string_view command) -> std::string
{
    constexpr std::string_view WRONG_ARG_ERR = "ERR wrong number of arguments for '{}' command";
    return xformat(WRONG_ARG_ERR, command);
}

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
        return std::make_unique<ErrorResponse>(INVALID_EXPIRY_ERR);

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
    return std::make_unique<ErrorResponse>(arguments_size_error("set"));
}

auto get(const arguments& args) -> response
{
    if (args.size() != 2) {
        log::error("GET command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("get"));
    }

    auto res = db.get(args[1]);
    if (!res) {
        log::info("GET command executed with key: {}, but key does not exist", args[1]);
        return std::make_unique<NullBulkStringResponse>();
    }

    if (auto* value = std::get_if<string_type>(&*res)) {
        log::info("GET command executed with key: {}, value found", args[1]);
        return std::make_unique<SingleBulkStringResponse>(*value);
    }

    log::error("GET command executed with key: {}, but WRONGTYPE Operation against a key holding "
               "the wrong kind of value",
               args[1]);
    return std::make_unique<ErrorResponse>(WRONG_TYPE_ERR);
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
    return std::make_unique<ErrorResponse>(arguments_size_error("ping"));
}

auto keys(const arguments& args) -> response
{
    if (args.size() < 2) {
        log::error("KEYS command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("keys"));
    }

    auto pattern = std::span{ args.begin() + 1, args.end() };
    log::info("KEYS command executed with pattern: {}", pattern);
    auto res = db.keys(pattern);

    BulkStringResponse response{};
    for (auto& item : res)
        response.add_record(std::make_shared<std::string>(item));

    log::info("KEYS command executed with pattern: {}, found {} keys", pattern, res.size());
    return std::make_unique<BulkStringResponse>(std::move(response));
}

auto mget(const arguments& args) -> response
{
    if (args.size() < 2) {
        log::error("MGET command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("mget"));
    }

    auto keys = std::span{ args.begin() + 1, args.end() };
    log::info("MGET command executed with keys: {}", keys);
    auto res = db.mget(keys);

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
    return std::make_unique<ErrorResponse>(arguments_size_error("flushdb"));
}

auto dbsize(const arguments& args) -> response
{
    if (args.size() != 1) {
        log::error("DBSIZE command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("dbsize"));
    }

    auto size = db.size();
    log::info("DBSIZE command executed, database size: {}", size);
    return std::make_unique<IntegralResponse>(size);
}

auto expire(const arguments& args) -> response
{
    if (args.size() != 3) {
        log::error("EXPIRE command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("expire"));
    }

    auto seconds = numeric_cast<std::uint64_t>(args[2]);
    if (!seconds)
        return std::make_unique<ErrorResponse>(INVALID_EXPIRY_ERR);

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
        return std::make_unique<ErrorResponse>(arguments_size_error("ttl"));
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
        return std::make_unique<ErrorResponse>(arguments_size_error("persist"));
    }

    auto key = args[1];
    if (db.persist(key)) {
        log::info("PERSIST command executed for key: {}, expiration removed", key);
        return std::make_unique<IntegralResponse>(1);
    }

    log::info("PERSIST command executed for key: {}, but key has no expiration", key);
    return std::make_unique<IntegralResponse>(0);
}

auto create_new_hash(const arguments& args) -> response
{
    log::info("HSET command executed with key: {}, but key does not exist, creating new hash",
              args[1]);

    auto hash = std::make_shared<std::unordered_map<std::string, string_type>>();
    int new_fields = 0;
    for (size_t i = 2; i + 1 < args.size(); i += 2) {
        auto [_, inserted] =
            hash->insert_or_assign(args[i], std::make_shared<std::string>(args[i + 1]));

        if (!inserted)
            ++new_fields;
    }

    db.set(args[1], std::move(hash));

    return std::make_unique<IntegralResponse>(new_fields);
}

auto add_to_existing_hash(const arguments& args, hash_type& container) -> response
{
    int new_fields = 0;
    for (size_t i = 2; i + 1 < args.size(); i += 2) {
        auto [_, inserted] =
            container->insert_or_assign(args[i], std::make_shared<std::string>(args[i + 1]));

        if (!inserted)
            ++new_fields;
    }

    log::info("HSET command executed with key: {}, updated {} fields, added {} new fields", args[1],
              container->size() - new_fields, new_fields);
    return std::make_unique<IntegralResponse>(new_fields);
}

auto hset(const arguments& args) -> response
{
    if (args.size() < 4 || args.size() % 2 != 0) {
        log::error("HSET command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("hset"));
    }

    auto res = db.get(args[1]);

    // 如果key不存在，那么就创建一个新的hash对象并插入字段值对
    if (!res)
        return create_new_hash(args);

    // 如果key存在且是hash类型，那么就更新hash中的字段值对
    if (auto* hash = std::get_if<hash_type>(&*res))
        return add_to_existing_hash(args, *hash);

    // 如果key存在但不是hash类型，那么就返回错误
    log::error("HSET command executed with key: {}, but WRONGTYPE Operation against a key holding "
               "the wrong kind of value",
               args[1]);
    return std::make_unique<ErrorResponse>(
        "WRONGTYPE Operation against a key holding the wrong kind of value");
}

auto get_from_hash(const arguments& args, hash_type& container) -> response
{
    const auto& field = args[2];
    auto iter = container->find(field);
    if (iter == container->end()) {
        log::info("HGET command executed with key: {}, field: {}, but field does not exist",
                  args[1], field);
        return std::make_unique<NullBulkStringResponse>();
    }

    log::info("HGET command executed with key: {}, field: {}, value found", args[1], field);
    return std::make_unique<SingleBulkStringResponse>(iter->second);
}

auto hget(const arguments& args) -> response
{
    if (args.size() != 3) {
        log::error("HGET command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("hget"));
    }

    auto res = db.get(args[1]);
    if (!res) {
        log::info("HGET command executed with key: {}, field: {}, but key does not exist", args[1],
                  args[2]);
        return std::make_unique<NullBulkStringResponse>();
    }

    if (auto* hash = std::get_if<hash_type>(&*res))
        return get_from_hash(args, *hash);

    log::error("HGET command executed with key: {}, field: {}, but WRONGTYPE Operation against a "
               "key holding the wrong kind of value",
               args[1], args[2]);
    return std::make_unique<ErrorResponse>(
        "WRONGTYPE Operation against a key holding the wrong kind of value");
}

auto get_all_from_hash(const arguments& args, hash_type& container) -> response
{
    auto response = std::make_unique<BulkStringResponse>();
    for (const auto& [field, value] : *container) {
        log::info("HGETALL command executed with key: {}, field: {}, value: {}", args[1], field,
                  *value);
        response->add_record(std::make_shared<std::string>(field));
        response->add_record(value);
    }

    log::info("HGETALL command executed with key: {}, total {} fields returned", args[1],
              response->size() / 2);
    return response;
}

auto hget_all(const arguments& args) -> response
{
    if (args.size() != 2) {
        log::error("HGETALL command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("hgetall"));
    }

    auto res = db.get(args[1]);
    if (!res) {
        log::info("HGETALL command executed with key: {}, but key does not exist", args[1]);
        return std::make_unique<BulkStringResponse>();
    }

    if (auto* hash = std::get_if<hash_type>(&*res))
        return get_all_from_hash(args, *hash);

    log::error("HGETALL command executed with key: {}, but WRONGTYPE Operation against a "
               "key holding the wrong kind of value",
               args[1]);
    return std::make_unique<ErrorResponse>(
        "WRONGTYPE Operation against a key holding the wrong kind of value");
}

auto create_new_list(const arguments& args) -> response
{
    log::info("LPUSH command executed with key: {}, but key does not exist, creating new list",
              args[1]);

    auto list = std::make_shared<list_type::element_type>();
    for (size_t i = 2; i < args.size(); ++i)
        list->push_front(std::make_shared<std::string>(args[i]));

    db.set(args[1], std::move(list));

    return std::make_unique<IntegralResponse>(args.size() - 2);
}

auto add_to_existing_list(const arguments& args, list_type& container) -> response
{
    for (size_t i = 2; i < args.size(); ++i)
        container->push_front(std::make_shared<std::string>(args[i]));

    log::info("LPUSH command executed with key: {}, added {} elements to existing list", args[1],
              args.size() - 2);
    return std::make_unique<IntegralResponse>(container->size());
}

auto lpush(const arguments& args) -> response
{
    if (args.size() < 3) {
        log::error("LPUSH command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("lpush"));
    }

    auto res = db.get(args[1]);
    if (!res)
        return create_new_list(args);

    if (auto* list = std::get_if<list_type>(&*res))
        return add_to_existing_list(args, *list);

    log::error("LPUSH command executed with key: {}, but WRONGTYPE Operation against a "
               "key holding the wrong kind of value",
               args[1]);
    return std::make_unique<ErrorResponse>(WRONG_TYPE_ERR);
}

auto lpop(const arguments& args) -> response
{
    if (args.size() != 2) {
        log::error("LPOP command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("lpop"));
    }

    auto res = db.get(args[1]);
    if (!res) {
        log::info("LPOP command executed with key: {}, but key does not exist", args[1]);
        return std::make_unique<NullBulkStringResponse>();
    }

    if (auto* list = std::get_if<list_type>(&*res)) {
        auto& container = *list;

        if (container->empty()) {
            log::info("LPOP command executed with key: {}, but list is empty", args[1]);
            return std::make_unique<NullBulkStringResponse>();
        }

        auto value = container->front();
        container->pop_front();

        log::info("LPOP command executed with key: {}, popped value: {}", args[1], *value);
        return std::make_unique<SingleBulkStringResponse>(value);
    }

    log::error("LPOP command executed with key: {}, but WRONGTYPE Operation against a "
               "key holding the wrong kind of value",
               args[1]);
    return std::make_unique<ErrorResponse>(WRONG_TYPE_ERR);
}

using handlers_table = std::unordered_map<std::string, handler>;
handlers_table handlers = { { "set", set },         { "get", get },       { "ping", ping },
                            { "keys", keys },       { "mget", mget },     { "flushdb", flushdb },
                            { "dbsize", dbsize },   { "expire", expire }, { "ttl", ttl },
                            { "persist", persist }, { "hget", hget },     { "hgetall", hget_all },
                            { "hset", hset },       { "lpush", lpush },   { "lpop", lpop } };

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