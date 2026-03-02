#include "redis_command_hash_table.h"

#include <base_log.h>

using namespace xin::base;

namespace {

using namespace xin::redis;

auto create_new_hash(const Arguments& args) -> ResponsePtr
{
    log::info("HSET command executed with key: {}, but key does not exist, creating new hash",
              args[1]);

    auto hash = std::make_shared<std::unordered_map<std::string, StringPtr>>();
    int new_fields = 0;
    for (size_t i = 2; i + 1 < args.size(); i += 2) {
        auto [_, inserted] =
            hash->insert_or_assign(args[i], std::make_shared<std::string>(args[i + 1]));

        if (inserted)
            ++new_fields;
    }

    db().set(args[1], std::move(hash));

    return std::make_unique<IntegralResponse>(new_fields);
}

auto add_to_existing_hash(const Arguments& args, HashType& container) -> ResponsePtr
{
    int new_fields = 0;
    for (size_t i = 2; i + 1 < args.size(); i += 2) {
        auto [_, inserted] =
            container.insert_or_assign(args[i], std::make_shared<std::string>(args[i + 1]));

        if (inserted)
            ++new_fields;
    }

    log::info("HSET command executed with key: {}, updated {} fields, added {} new fields", args[1],
              container.size() - new_fields, new_fields);
    return std::make_unique<IntegralResponse>(new_fields);
}

auto get_from_hash(const Arguments& args, HashType& container) -> ResponsePtr
{
    const auto& field = args[2];
    auto iter = container.find(field);
    if (iter == container.end()) {
        log::info("HGET command executed with key: {}, field: {}, but field does not exist",
                  args[1], field);
        return std::make_unique<NullBulkStringResponse>();
    }

    log::info("HGET command executed with key: {}, field: {}, value found", args[1], field);
    return std::make_unique<SingleBulkStringResponse>(iter->second);
}

auto get_all_from_hash(const Arguments& args, HashType& container) -> ResponsePtr
{
    auto response = std::make_unique<BulkStringResponse>();
    for (const auto& [field, value] : container) {
        log::info("HGETALL command executed with key: {}, field: {}, value: {}", args[1], field,
                  *value);
        response->add_record(std::make_shared<std::string>(field));
        response->add_record(value);
    }

    log::info("HGETALL command executed with key: {}, total {} fields returned", args[1],
              response->size() / 2);
    return response;
}

} // namespace

namespace xin::redis {

auto hash_table_commands::set(const Arguments& args) -> ResponsePtr
{
    if (args.size() < 4 || args.size() % 2 != 0) {
        log::error("HSET command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("hset"));
    }

    auto res = db().get(args[1]);

    // 如果key不存在，那么就创建一个新的hash对象并插入字段值对
    if (!res)
        return create_new_hash(args);

    // 如果key存在且是hash类型，那么就更新hash中的字段值对
    if (auto* hash = std::get_if<HashPtr>(&*res))
        return add_to_existing_hash(args, **hash);

    // 如果key存在但不是hash类型，那么就返回错误
    log::error("HSET command executed with key: {}, but WRONGTYPE Operation against a key holding "
               "the wrong kind of value",
               args[1]);
    return std::make_unique<ErrorResponse>(
        "WRONGTYPE Operation against a key holding the wrong kind of value");
}

auto hash_table_commands::get(const Arguments& args) -> ResponsePtr
{
    if (args.size() != 3) {
        log::error("HGET command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("hget"));
    }

    auto res = db().get(args[1]);
    if (!res) {
        log::info("HGET command executed with key: {}, field: {}, but key does not exist", args[1],
                  args[2]);
        return std::make_unique<NullBulkStringResponse>();
    }

    if (auto* hash = std::get_if<HashPtr>(&*res))
        return get_from_hash(args, **hash);

    log::error("HGET command executed with key: {}, field: {}, but WRONGTYPE Operation against a "
               "key holding the wrong kind of value",
               args[1], args[2]);
    return std::make_unique<ErrorResponse>(
        "WRONGTYPE Operation against a key holding the wrong kind of value");
}

auto hash_table_commands::get_all(const Arguments& args) -> ResponsePtr
{
    if (args.size() != 2) {
        log::error("HGETALL command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("hgetall"));
    }

    auto res = db().get(args[1]);
    if (!res) {
        log::info("HGETALL command executed with key: {}, but key does not exist", args[1]);
        return std::make_unique<BulkStringResponse>();
    }

    if (auto* hash = std::get_if<HashPtr>(&*res))
        return get_all_from_hash(args, **hash);

    log::error("HGETALL command executed with key: {}, but WRONGTYPE Operation against a "
               "key holding the wrong kind of value",
               args[1]);
    return std::make_unique<ErrorResponse>(
        "WRONGTYPE Operation against a key holding the wrong kind of value");
}

} // namespace xin::redis