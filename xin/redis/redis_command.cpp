#include "redis_command.h"

#include <base_formats.h>
#include <base_log.h>
#include <base_string_utility.h>
#include <fmt/core.h>
#include <redis_application_context.h>
#include <redis_command_common.h>
#include <redis_command_define.h>
#include <redis_command_hash_table.h>
#include <redis_command_list.h>
#include <redis_command_sorted_set.h>
#include <redis_command_string.h>
#include <redis_response.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace {

using namespace xin::redis;
using namespace xin::base;

enum class Type : std::uint8_t { Read, Write };

struct Command {
    Type type;
    Handler handler;
};

using handlers_table = std::unordered_map<std::string, Command>;
handlers_table handlers = {
    { "set", { .type = Type::Write, .handler = string_commands::set } },
    { "get", { .type = Type::Read, .handler = string_commands::get } },
    { "ping", { .type = Type::Read, .handler = common_commands::ping } },
    { "keys", { .type = Type::Read, .handler = common_commands::keys } },
    { "mget", { .type = Type::Read, .handler = common_commands::mget } },
    { "flushdb", { .type = Type::Write, .handler = common_commands::flushdb } },
    { "dbsize", { .type = Type::Read, .handler = common_commands::dbsize } },
    { "expire", { .type = Type::Write, .handler = common_commands::expire } },
    { "ttl", { .type = Type::Read, .handler = common_commands::ttl } },
    { "persist", { .type = Type::Write, .handler = common_commands::persist } },
    { "hget", { .type = Type::Read, .handler = hash_table_commands::get } },
    { "hgetall", { .type = Type::Read, .handler = hash_table_commands::get_all } },
    { "hset", { .type = Type::Write, .handler = hash_table_commands::set } },
    { "lpush", { .type = Type::Write, .handler = list_commands::push } },
    { "lpop", { .type = Type::Write, .handler = list_commands::pop } },
    { "lrange", { .type = Type::Read, .handler = list_commands::range } },
    { "zadd", { .type = Type::Write, .handler = sorted_set_commands::add } },
    { "zrange", { .type = Type::Read, .handler = sorted_set_commands::range } },
};

} // namespace

namespace xin::redis {

auto commands::dispatch(const Arguments& args) -> ResponsePtr
{
    if (args.empty())
        return std::make_unique<ErrorResponse>("ERR empty command");

    using namespace xin::base;
    auto it = handlers.find(strings::to_lowercase(args[0]));
    if (it == handlers.end())
        return std::make_unique<ErrorResponse>(fmt::format("ERR unknown command '{}'", args[0]));

    if (it->second.type == Type::Write && !application_context::replaying_aof)
        application_context::aof_logger.append(resp::serialize(args));

    return it->second.handler(args);
}

} // namespace xin::redis