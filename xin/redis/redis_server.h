#ifndef XIN_REDIS_SERVER_H
#define XIN_REDIS_SERVER_H

#include <asio.hpp>
#include <asio/any_io_executor.hpp>
#include <asio/awaitable.hpp>
#include <asio/strand.hpp>
#include <redis_application_context.h>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace xin::redis {

class Server {
public:
    using Port = std::uint16_t;
    using Size = std::size_t;

    explicit Server(Port port, Size thread_count = 1);

    Server(const Server&) = delete;
    auto operator=(const Server&) -> Server& = delete;

    Server(Server&&) = delete;
    auto operator=(Server&&) -> Server& = delete;

    ~Server() = default;

    void start();

private:
    using Acceptor = asio::ip::tcp::acceptor;
    using Strand = asio::strand<asio::any_io_executor>;

    Port port_;
    Size thread_count_;
    std::unique_ptr<Acceptor> acceptor_;
    asio::io_context io_context_;
    Strand strand_;

    auto listen() -> asio::awaitable<void>;

    auto dispatch(asio::ip::tcp::socket socket) -> asio::awaitable<void>;

    auto erase_expired_data() -> asio::awaitable<void>;

    static void load_aof();
};

} // namespace xin::redis

#endif // XIN_REDIS_SERVER_H
