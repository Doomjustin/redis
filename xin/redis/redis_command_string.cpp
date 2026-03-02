#include "redis_command_string.h"

#include <base_log.h>
#include <base_string_utility.h>

using namespace xin::base;

namespace xin::redis {

auto set_with_expiry(const Arguments& args) -> ResponsePtr
{
    auto expiry = numeric_cast<std::uint64_t>(args[4]);
    if (!expiry)
        return std::make_unique<ErrorResponse>(INVALID_INTEGRAL_ERR);

    db().set(args[1], std::make_shared<std::string>(args[2]), *expiry);
    log::info("SET command with expiry executed with key: {}, value: {}, expire time: {} seconds",
              args[1], args[2], *expiry);
    return std::make_unique<SimpleStringResponse>("OK");
}

auto set_persist(const Arguments& args) -> ResponsePtr
{
    db().set(args[1], std::make_shared<std::string>(args[2]));
    log::info("SET command executed with key: {} and value: {}", args[1], args[2]);
    return std::make_unique<SimpleStringResponse>("OK");
}

auto string_commands::set(const Arguments& args) -> ResponsePtr
{
    if (args.size() == 3)
        return set_persist(args);

    if (args.size() == 5 && strings::to_lowercase(args[3]) == "ex")
        return set_with_expiry(args);

    log::error("SET command received wrong number of arguments or invalid option: {}", args);
    return std::make_unique<ErrorResponse>(arguments_size_error("set"));
}

auto string_commands::get(const Arguments& args) -> ResponsePtr
{
    if (args.size() != 2) {
        log::error("GET command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("get"));
    }

    auto res = db().get(args[1]);
    if (!res) {
        log::info("GET command executed with key: {}, but key does not exist", args[1]);
        return std::make_unique<NullBulkStringResponse>();
    }

    if (auto* value = std::get_if<StringPtr>(&*res)) {
        log::info("GET command executed with key: {}, value found", args[1]);
        return std::make_unique<SingleBulkStringResponse>(*value);
    }

    log::error("GET command executed with key: {}, but WRONGTYPE Operation against a key holding "
               "the wrong kind of value",
               args[1]);
    return std::make_unique<ErrorResponse>(WRONG_TYPE_ERR);
}

} // namespace xin::redis