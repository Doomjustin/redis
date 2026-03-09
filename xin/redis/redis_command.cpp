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

// select index
// Change the selected database for the current connection. The index of the selected database
// is specified by the index argument. The default database is 0, and the default number of
// databases is 16.
auto select(std::size_t& index, const Arguments& args) -> ResponsePtr
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

std::size_t last_db_index = 0;

} // namespace

namespace xin::redis {

auto commands::dispatch(std::size_t& index, const Arguments& args) -> ResponsePtr
{
    if (args.empty())
        return std::make_unique<ErrorResponse>("ERR empty command");

    using namespace xin::base;

    // SELECT命令是特殊的，我们单独处理，确保它在AOF中正确记录数据库切换
    auto command = strings::to_lowercase(args[0]);
    if (command == "select")
        return select(index, args);

    auto it = handlers.find(command);
    if (it == handlers.end())
        return std::make_unique<ErrorResponse>(xformat("ERR unknown command '{}'", args[0]));

    if (it->second.type == Type::Write && !application_context::replaying_aof) {
        if (last_db_index != index) {
            auto select_args = Arguments{ "SELECT", std::to_string(index) };
            application_context::aof_logger.append(resp::serialize(select_args));
            last_db_index = index;
        }

        application_context::aof_logger.append(resp::serialize(args));
    }

    return it->second.handler(index, args);
}

} // namespace xin::redis