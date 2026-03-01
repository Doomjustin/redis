#ifndef REDIS_SERIALIZATION_PROTOCOL_H
#define REDIS_SERIALIZATION_PROTOCOL_H

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace xin::redis {

// 这是一个 RESP 协议解析器，设计为状态机，支持增量解析。
// 需要手动重置状态以解析下一条命令。
class RESPParser {
public:
    using arguments = std::vector<std::string>;

    enum class Error : std::uint8_t {
        Waiting, // 数据不够，等待更多数据
        Error    // 解析错误
    };

    auto parse(std::span<const char>& buf) -> std::expected<arguments, Error>;

    void reset();

private:
    enum class State : std::uint8_t {
        Start,
        ReadingArraySize,
        ReadingBulkPrefix,
        ReadingBulkSize,
        ReadingBulkData
    };

    State state_ = State::Start;
    int expected_args_count_ = 0;
    int current_arg_length_ = 0;
    arguments args_;

    auto read_start(std::span<const char>& buf) -> std::expected<void, Error>;

    auto read_array_size(std::span<const char>& buf) -> std::expected<void, Error>;

    auto read_bulk_prefix(std::span<const char>& buf) -> std::expected<void, Error>;

    auto read_bulk_size(std::span<const char>& buf) -> std::expected<void, Error>;

    auto parse_reading_bulk_data(std::span<const char>& buf) -> std::expected<void, Error>;
};

} // namespace xin::redis

#endif // REDIS_SERIALIZATION_PROTOCOL_H