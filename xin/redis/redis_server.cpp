#include "redis_server.h"

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/error.hpp>
#include <asio/steady_timer.hpp>
#include <base_log.h>
#include <redis_application_context.h>
#include <redis_command.h>
#include <redis_command_define.h>
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
        co_spawn(context_, erase_expired_data(), detached);
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
        if (e.code() == asio::error::eof || e.code() == asio::error::connection_reset)
            co_return;

        log::error("Session error: {}", e.what());
    }
}

auto Server::erase_expired_data() -> asio::awaitable<void>
{
    steady_timer timer{ co_await this_coro::executor };

    while (true) {
        using namespace std::chrono_literals;
        timer.expires_after(100ms);

        co_await timer.async_wait(use_awaitable);
        log::debug("Erasing expired keys...");

        // 遍历所有数据库，删除过期的键
        for (std::size_t index = 0; auto& db : application_context::databases) {
            auto erased_keys = db.expired_keys();

            if (erased_keys.empty()) {
                log::debug("DB{} have no expired keys.", index);
            }
            else {
                log::debug("Erased keys from DB{}: {}", index, erased_keys);
                // 通过执行删除命令，确保aof可以记录删除操作
                erased_keys.insert(erased_keys.begin(), "del");
                commands::dispatch(index, erased_keys);
            }

            ++index;
        }
    }
}

} // namespace xin::redis
