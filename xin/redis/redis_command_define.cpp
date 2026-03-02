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

auto normalize_range(std::int64_t start, std::int64_t stop, std::size_t length) -> Range
{
    if (start < 0)
        start += length;

    if (stop < 0)
        stop += length;

    start = std::max(start, static_cast<std::int64_t>(0));
    stop = std::min(stop, static_cast<std::int64_t>(length - 1));

    return { .start = start, .stop = stop };
}

} // namespace xin::redis