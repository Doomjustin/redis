#include "redis_serialization_protocl.h"

#include <base_formats.h>

#include <algorithm>
#include <charconv>
#include <expected>
#include <optional>
#include <span>

namespace {

constexpr std::string_view CRLF = "\r\n";

// RESP 协议中，数组以 '*' 开头，后跟元素数量，每个元素以 '$' 开头，后跟元素长度和数据
// 但是客户端永远以数组形式发送命令，所以我们只需要处理数组和 bulk string 两种类型即可
// example: *2\r\n$3\r\nGET\r\n$3\r\nkey\r\n
constexpr char ARRAY_PREFIX = '*';
constexpr char BULK_STRING_PREFIX = '$';
constexpr char SIMPLE_STRING_PREFIX = '+';
constexpr char ERROR_PREFIX = '-';
constexpr char INTEGER_PREFIX = ':';

auto read_integral(std::span<const char>& buf) -> std::optional<long>
{
    auto match = std::ranges::search(buf, CRLF);
    if (match.begin() == buf.end())
        return {};

    auto line_len = static_cast<std::size_t>(match.begin() - buf.begin());
    if (line_len == 0)
        return {};

    long value = 0;
    const char* first = buf.data();
    const char* last = first + line_len;
    auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr != last)
        return {};

    buf = buf.subspan(line_len + CRLF.size());
    return value;
}

} // namespace

namespace xin::redis {

auto RESPParser::parse(std::span<const char>& buf) -> std::expected<arguments, Error>
{
    while (!buf.empty()) {
        switch (state_) {
        case State::Start:
            if (auto res = read_start(buf); !res)
                return std::unexpected(res.error());
            break;
        case State::ReadingArraySize:
            if (auto res = read_array_size(buf); !res)
                return std::unexpected(res.error());
            break;
        case State::ReadingBulkPrefix:
            // 已经读完了所有参数
            if (args_.size() == expected_args_count_)
                return args_;

            if (auto res = read_bulk_prefix(buf); !res)
                return std::unexpected(res.error());

            break;
        case State::ReadingBulkSize:
            if (auto res = read_bulk_size(buf); !res)
                return std::unexpected(res.error());
            break;
        case State::ReadingBulkData:
            if (auto res = parse_reading_bulk_data(buf); !res)
                return std::unexpected(res.error());

            // 如果最后一个参数刚好读完，直接返回结果，避免多余的循环
            if (args_.size() == expected_args_count_)
                return args_;

            break;
        }
    }

    return std::unexpected(Error::Waiting);
}

auto RESPParser::read_start(std::span<const char>& buf) -> std::expected<void, Error>
{
    if (buf.front() != ARRAY_PREFIX)
        return std::unexpected(Error::Error);

    buf = buf.subspan(1);
    state_ = State::ReadingArraySize;
    return {};
}

auto RESPParser::read_array_size(std::span<const char>& buf) -> std::expected<void, Error>
{
    auto len = read_integral(buf);
    if (!len)
        return std::unexpected(Error::Error);

    expected_args_count_ = *len;
    args_.reserve(expected_args_count_);
    state_ = State::ReadingBulkPrefix;
    return {};
}

auto RESPParser::read_bulk_prefix(std::span<const char>& buf) -> std::expected<void, Error>
{
    if (buf.front() != BULK_STRING_PREFIX)
        return std::unexpected(Error::Error);

    buf = buf.subspan(1);
    state_ = State::ReadingBulkSize;

    return {};
}

auto RESPParser::read_bulk_size(std::span<const char>& buf) -> std::expected<void, Error>
{
    auto len = read_integral(buf);
    if (!len)
        return std::unexpected(Error::Waiting);

    current_arg_length_ = *len;
    state_ = State::ReadingBulkData;
    return {};
}

auto RESPParser::parse_reading_bulk_data(std::span<const char>& buf) -> std::expected<void, Error>
{
    std::size_t line_size = current_arg_length_ + CRLF.size();
    if (buf.size() < line_size)
        return std::unexpected(Error::Waiting);

    if (std::string_view{ &buf[current_arg_length_], 2 } != CRLF)
        return std::unexpected(Error::Error);

    args_.emplace_back(buf.data(), current_arg_length_);
    buf = buf.subspan(line_size);
    state_ = State::ReadingBulkPrefix;
    return {};
}

auto RESPParser::reset() -> void
{
    state_ = State::Start;
    expected_args_count_ = 0;
    current_arg_length_ = 0;
    args_.clear();
}

} // namespace xin::redis