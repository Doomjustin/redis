#include "redis_command_define.h"

#include <base_formats.h>

using namespace xin::base;

namespace xin::redis {

auto db(int index) -> Database&
{
    static Database instance;
    return instance;
}

auto arguments_size_error(std::string_view command) -> std::string
{
    constexpr std::string_view WRONG_ARG_ERR = "ERR wrong number of arguments for '{}' command";
    return xformat(WRONG_ARG_ERR, command);
}

} // namespace xin::redis