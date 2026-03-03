#ifndef XIN_REDIS_SERVER_H
#define XIN_REDIS_SERVER_H

#include <asio.hpp>
#include <asio/awaitable.hpp>

#include <cstdint>
#include <memory>

namespace xin::redis {

class Server {
public:
    using PortType = std::uint16_t;

    Server(PortType port, int thread_count = 1);

    Server(const Server&) = delete;
    auto operator=(const Server&) -> Server& = delete;

    Server(Server&&) = delete;
    auto operator=(Server&&) -> Server& = delete;

    ~Server() = default;

    void start();

private:
    using AcceptorType = asio::ip::tcp::acceptor;

    PortType port_;
    std::unique_ptr<AcceptorType> acceptor_;
    asio::io_context context_;

    auto listen() -> asio::awaitable<void>;

    static auto dispatch(asio::ip::tcp::socket socket) -> asio::awaitable<void>;
};

} // namespace xin::redis

#endif // XIN_REDIS_SERVER_H
