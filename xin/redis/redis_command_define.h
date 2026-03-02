#ifndef XIN_REDIS_COMMAND_DEFINE_H
#define XIN_REDIS_COMMAND_DEFINE_H

#include <base_log.h>
#include <redis_response.h>
#include <redis_serialization_protocl.h>
#include <redis_storage.h>

namespace xin::redis {

using Arguments = RESPParser::arguments;
using ResponsePtr = std::unique_ptr<Response>;
using Handler = std::function<ResponsePtr(const Arguments&)>;
using StringType = Database::StringType;
using StringPtr = Database::StringPtr;
using HashType = Database::HashType;
using HashPtr = Database::HashPtr;
using ListType = Database::ListType;
using ListPtr = Database::ListPtr;

// 尚未实现的命令占位符定义
auto db(int index = 0) -> Database&;

static constexpr std::string_view WRONG_TYPE_ERR =
    "WRONGTYPE Operation against a key holding the wrong kind of value";

static constexpr std::string_view INVALID_INTEGRAL_ERR =
    "ERR value is not an integer or out of range";

auto arguments_size_error(std::string_view command) -> std::string;

template<typename T>
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

struct Range {
    std::int64_t start;
    std::int64_t stop;
};

auto normalize_range(std::int64_t start, std::int64_t stop, std::size_t length) -> Range;

} // namespace xin::redis

#endif // XIN_REDIS_COMMAND_DEFINE_H