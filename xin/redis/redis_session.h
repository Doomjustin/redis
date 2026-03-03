#ifndef XIN_REDIS_SESSION_H
#define XIN_REDIS_SESSION_H

#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <redis_response.h>
#include <redis_serialization_protocl.h>

#include <array>
#include <memory>
#include <vector>

namespace xin::redis {

class Session {
public:
    explicit Session(asio::ip::tcp::socket socket);

    auto start() -> asio::awaitable<void>;

private:
    static constexpr std::size_t BUFFER_SIZE = 8192;

    asio::ip::tcp::socket socket_;
    std::array<char, BUFFER_SIZE> buffer_{};
    std::size_t write_idx_ = 0;
    RESPParser parser_{};
    std::vector<std::unique_ptr<Response>> responses;
};

} // namespace xin::redis

#endif // XIN_REDIS_SESSION_H
