#ifndef XIN_REDIS_COMMAND_DEFINE_H
#define XIN_REDIS_COMMAND_DEFINE_H

#include "redis_response.h"
#include "redis_serialization_protocl.h"
#include "redis_storage.h"

#include <base_log.h>

namespace xin::redis {

using arguments = RESPParser::arguments;
using response = std::unique_ptr<Response>;
using handler = std::function<response(const arguments&)>;
using string_type = Database::string_type;
using hash_type = Database::hash_type;
using list_type = Database::list_type;

// 尚未实现的命令占位符定义
auto db(int index = 0) -> Database&;

static constexpr std::string_view WRONG_TYPE_ERR =
    "WRONGTYPE Operation against a key holding the wrong kind of value";

static constexpr std::string_view INVALID_EXPIRY_ERR =
    "ERR value is not an integer or out of range";

auto arguments_size_error(std::string_view command) -> std::string;

template<std::integral T>
auto numeric_cast(std::string_view str) -> std::optional<T>
{
    T seconds;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), seconds);
    if (ec != std::errc()) {
        xin::base::log::error("EXPIRE command received invalid expiration time: {}", str);
        return {};
    }

    return seconds;
}

} // namespace xin::redis

#endif // XIN_REDIS_COMMAND_DEFINE_H