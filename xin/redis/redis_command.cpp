#include "redis_command.h"

#include "redis_command_common.h"
#include "redis_command_define.h"
#include "redis_command_hash_table.h"
#include "redis_command_list.h"
#include "redis_command_set.h"
#include "redis_command_string.h"
#include "redis_response.h"

#include <base_formats.h>
#include <base_log.h>
#include <base_string_utility.h>
#include <fmt/core.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace {

using namespace xin::redis;
using namespace xin::base;

using handlers_table = std::unordered_map<std::string, handler>;
handlers_table handlers = {
    { "set", string_commands::set },       { "get", string_commands::get },
    { "ping", common_commands::ping },     { "keys", common_commands::keys },
    { "mget", common_commands::mget },     { "flushdb", common_commands::flushdb },
    { "dbsize", common_commands::dbsize }, { "expire", common_commands::expire },
    { "ttl", common_commands::ttl },       { "persist", common_commands::persist },
    { "hget", hash_table_commands::get },  { "hgetall", hash_table_commands::get_all },
    { "hset", hash_table_commands::set },  { "lpush", list_commands::push },
    { "lpop", list_commands::pop },        { "lrange", list_commands::range }
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