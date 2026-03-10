#ifndef XIN_REDIS_SERVER_H
#define XIN_REDIS_SERVER_H

#include <asio.hpp>
#include <asio/awaitable.hpp>

#include <cstdint>
#include <memory>

namespace xin::redis {

class Server {
public:
    using Port = std::uint16_t;

    Server(Port port, int thread_count = 1);

    Server(const Server&) = delete;
    auto operator=(const Server&) -> Server& = delete;

    Server(Server&&) = delete;
    auto operator=(Server&&) -> Server& = delete;

    ~Server() = default;

    void start();

private:
    using Acceptor = asio::ip::tcp::acceptor;

    Port port_;
    std::unique_ptr<Acceptor> acceptor_;
    asio::io_context context_;

    auto listen() -> asio::awaitable<void>;

    static auto dispatch(asio::ip::tcp::socket socket) -> asio::awaitable<void>;

    static auto erase_expired_data() -> asio::awaitable<void>;
};

} // namespace xin::redis

#endif // XIN_REDIS_SERVER_H
