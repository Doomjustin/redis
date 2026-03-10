#ifndef XIN_REDIS_SESSION_H
#define XIN_REDIS_SESSION_H

#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <redis_application_context.h>
#include <redis_response.h>
#include <redis_serialization_protocl.h>

#include <array>
#include <memory>
#include <vector>

namespace xin::redis {

class Session {
public:
    Session(asio::ip::tcp::socket socket, asio::strand<asio::any_io_executor> strand);

    auto run() -> asio::awaitable<void>;

private:
    static constexpr std::size_t BUFFER_SIZE = 8192;
    using Strand = asio::strand<asio::any_io_executor>;

    asio::ip::tcp::socket socket_;
    asio::strand<asio::any_io_executor> strand_;
    std::array<char, BUFFER_SIZE> buffer_{};
    std::size_t write_idx_ = 0;
    std::size_t index_ = 0;
    RESPParser parser_{};
    std::vector<std::unique_ptr<Response>> responses;
};

} // namespace xin::redis

#endif // XIN_REDIS_SESSION_H
