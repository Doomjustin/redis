#include "redis_command.h"

#include "redis_response.h"
#include "redis_storage.h"

#include <base_log.h>
#include <base_string_utility.h>
#include <fmt/core.h>

#include <string>
#include <unordered_map>

namespace {

using namespace xin::redis;
using namespace xin::base;
using arguments = commands::arguments;
using response = commands::response;
using handler = std::function<response(const arguments&)>;

xin::redis::Database db{};

auto set(const arguments& args) -> response
{
    if (args.size() != 3) {
        log::error("SET command requires exactly 2 arguments, but got {}", args.size() - 1);
        return std::make_unique<ErrorResponse>("ERR wrong number of arguments for 'set' command");
    }

    db.set(args[1], std::make_shared<std::string>(args[2]));
    log::info("SET command executed with key: {} and value: {}", args[1], args[2]);
    return std::make_unique<SimpleStringResponse>("OK");
}

auto get(const arguments& args) -> response
{
    if (args.size() != 2) {
        log::error("GET command requires exactly 1 argument, but got {}", args.size() - 1);
        return std::make_unique<ErrorResponse>("ERR wrong number of arguments for 'get' command");
    }

    auto res = db.get(args[1]);
    if (!res) {
        log::info("GET command executed with key: {}, but key does not exist", args[1]);
        return std::make_unique<NullBulkStringResponse>();
    }

    SingleBulkStringResponse response{ *res };
    log::info("GET command executed with key: {} and value: {}", args[1], *res);
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

    log::error("PING command received wrong number of arguments: {}, expected 0 or 1",
               args.size() - 1);
    return std::make_unique<ErrorResponse>("ERR wrong number of arguments for 'ping' command");
}

auto keys(const arguments& args) -> response
{
    if (args.size() < 2) {
        log::error("KEYS command received wrong number of arguments: {}", args.size() - 1);
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
        log::error("MGET command received wrong number of arguments: {}", args.size() - 1);
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

using handlers_table = std::unordered_map<std::string, handler>;
handlers_table handlers = {
    { "set", set }, { "get", get }, { "ping", ping }, { "keys", keys }, { "mget", mget }
};

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