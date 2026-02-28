#include "redis_command.h"

#include "redis_response.h"
#include "redis_storage.h"

#include <base_string_utility.h>
#include <fmt/core.h>

#include <string>
#include <unordered_map>

namespace {

using xin::redis::commands;

std::unordered_map<std::string, commands::handler> handlers = { { "set", commands::set },
                                                                { "get", commands::get },
                                                                { "ping", commands::ping },
                                                                { "command", commands::command } };

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

auto commands::set(const arguments& args) -> response
{
    if (args.size() != 3)
        return std::make_unique<ErrorResponse>("ERR wrong number of arguments for 'set' command");

    storage.insert_or_assign(args[1], std::make_shared<std::string>(args[2]));
    return std::make_unique<SimpleStringResponse>("OK");
}

auto commands::get(const arguments& args) -> response
{
    if (args.size() != 2)
        return std::make_unique<ErrorResponse>("ERR wrong number of arguments for 'get' command");

    auto it = storage.find(args[1]);
    if (it == storage.end())
        return std::make_unique<NullBulkStringResponse>();

    BulkStringResponse res{};
    res.add_content(it->second);
    return std::make_unique<BulkStringResponse>(std::move(res));
}

auto commands::ping(const arguments& args) -> response
{
    if (args.size() == 1)
        return std::make_unique<SimpleStringResponse>("PONG");

    if (args.size() == 2)
        return std::make_unique<SimpleStringResponse>(args[1]);

    return std::make_unique<ErrorResponse>("ERR wrong number of arguments for 'ping' command");
}

auto commands::command(const arguments& args) -> response
{
    return std::make_unique<ErrorResponse>("ERR unknown command 'command'");
}

} // namespace xin::redis