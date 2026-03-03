#include "redis_server.h"

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <base_log.h>
#include <redis_session.h>

using xin::base::log;
using namespace asio;

namespace xin::redis {

Server::Server(PortType port, int thread_count)
    : port_{ port }
    , context_{ thread_count }
{
}

void Server::start()
{
    try {
        co_spawn(context_, listen(), detached);
        context_.run();
    }
    catch (const std::exception& e) {
        log::error("Server error: {}", e.what());
    }
}

auto Server::listen() -> asio::awaitable<void>
{
    acceptor_ = std::make_unique<AcceptorType>(co_await this_coro::executor,
                                               ip::tcp::endpoint(ip::tcp::v4(), port_));
    log::info("Listening on port {}", port_);

    while (true) {
        auto socket = co_await acceptor_->async_accept(use_awaitable);
        // 启动一个新的协程处理这个连接，detached
        // 表示我们不关心这个协程什么时候结束（它自己会管自己）
        co_spawn(acceptor_->get_executor(), dispatch(std::move(socket)), detached);
    }
}

auto Server::dispatch(asio::ip::tcp::socket socket) -> asio::awaitable<void>
{
    auto session = Session{ std::move(socket) };
    try {
        co_await session.start();
    }
    catch (const asio::system_error& e) {
        log::error("Session error: {}", e.what());
    }
}

} // namespace xin::redis
