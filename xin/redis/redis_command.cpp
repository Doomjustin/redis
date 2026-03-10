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

using Commands = std::unordered_map<std::string, Command>;
Commands commands_table = {
    { "set", { .type = Type::Write, .handler = string_commands::set } },
    { "get", { .type = Type::Read, .handler = string_commands::get } },
    { "ping", { .type = Type::Read, .handler = common_commands::ping } },
    { "keys", { .type = Type::Read, .handler = common_commands::keys } },
    { "mget", { .type = Type::Read, .handler = common_commands::mget } },
    { "del", { .type = Type::Write, .handler = common_commands::del } },
    { "flushdb", { .type = Type::Write, .handler = common_commands::flushdb } },
    { "dbsize", { .type = Type::Read, .handler = common_commands::dbsize } },
    { "expire", { .type = Type::Write, .handler = common_commands::expire } },
    { "ttl", { .type = Type::Read, .handler = common_commands::ttl } },
    { "persist", { .type = Type::Write, .handler = common_commands::persist } },
    { "exists", { .type = Type::Read, .handler = common_commands::exists } },
    { "hget", { .type = Type::Read, .handler = hash_table_commands::get } },
    { "hgetall", { .type = Type::Read, .handler = hash_table_commands::get_all } },
    { "hset", { .type = Type::Write, .handler = hash_table_commands::set } },
    { "lpush", { .type = Type::Write, .handler = list_commands::push } },
    { "lpop", { .type = Type::Write, .handler = list_commands::pop } },
    { "lrange", { .type = Type::Read, .handler = list_commands::range } },
    { "zadd", { .type = Type::Write, .handler = sorted_set_commands::add } },
    { "zrange", { .type = Type::Read, .handler = sorted_set_commands::range } },
};

std::size_t last_db_index = 0;

} // namespace

namespace xin::redis {

auto commands::dispatch(std::size_t& index, const Arguments& args) -> ResponsePtr
{
    if (args.empty())
        return std::make_unique<ErrorResponse>("ERR empty command");

    auto command = strings::to_lowercase(args[0]);
    if (command == "select")
        return common_commands::select(index, args);

    auto it = commands_table.find(command);
    if (it == commands_table.end())
        return std::make_unique<ErrorResponse>(xformat("ERR unknown command '{}'", args[0]));

    // 避免重建时二次写入AOF日志
    if (it->second.type == Type::Write && !application_context::replaying_aof) {
        if (last_db_index != index) {
            auto select_args = Arguments{ "SELECT", std::to_string(index) };
            application_context::aof_logger.append(resp::serialize(select_args));
            last_db_index = index;
        }

        application_context::aof_logger.append(resp::serialize(args));
    }

    return it->second.handler(application_context::db(index), args);
}

} // namespace xin::redis