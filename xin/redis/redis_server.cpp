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

#include <thread>

using xin::base::log;

namespace xin::redis {

Server::Server(Port port, Size thread_count)
    : port_{ port }
    , thread_count_{ thread_count }
    , io_context_{ static_cast<int>(thread_count) }
    , strand_{ asio::make_strand(io_context_) }
{
}

void Server::start()
{
    try {
        load_aof();

        co_spawn(io_context_, listen(), asio::detached);
        co_spawn(io_context_, erase_expired_data(), asio::detached);

        std::vector<std::jthread> threads(thread_count_);
        for (int i = 0; i < thread_count_; ++i)
            threads[i] = std::jthread{ [this]() { io_context_.run(); } };
    }
    catch (const std::exception& e) {
        log::error("Server error: {}", e.what());
    }
}

auto Server::listen() -> asio::awaitable<void>
{
    auto endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port_);
    acceptor_ = std::make_unique<Acceptor>(co_await asio::this_coro::executor, std::move(endpoint));
    log::info("Listening on port {}", port_);

    while (true) {
        auto socket = co_await acceptor_->async_accept(asio::use_awaitable);
        co_spawn(acceptor_->get_executor(), dispatch(std::move(socket)), asio::detached);
    }
}

auto Server::dispatch(asio::ip::tcp::socket socket) -> asio::awaitable<void>
{
    auto session = Session{ std::move(socket), strand_ };
    try {
        co_await session.run();
    }
    catch (const asio::system_error& e) {
        if (e.code() == asio::error::eof || e.code() == asio::error::connection_reset)
            co_return;

        log::error("Session error: {}", e.what());
    }
}

auto Server::erase_expired_data() -> asio::awaitable<void>
{
    asio::steady_timer timer{ co_await asio::this_coro::executor };

    while (true) {
        using namespace std::chrono_literals;
        timer.expires_after(100ms);

        co_await timer.async_wait(asio::use_awaitable);
        log::debug("Erasing expired keys...");

        // 遍历所有数据库，删除过期的键
        for (std::size_t index = 0; auto& db : application_context::databases) {
            co_await asio::dispatch(strand_, asio::use_awaitable);

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

void Server::load_aof()
{
    std::ifstream file{ std::string(application_context::AOF_FILE_PATH), std::ios::binary };
    if (!file.is_open())
        return;

    std::string content{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
    if (content.empty())
        return;

    application_context::replaying_aof = true;

    std::span<const char> buf{ content.data(), content.size() };
    RESPParser parser;
    std::size_t succeed_count = 0;
    std::size_t failed_count = 0;
    std::size_t index = 0;

    while (!buf.empty()) {
        auto result = parser.parse(buf);
        if (result) {
            commands::dispatch(index, *result);
            ++succeed_count;
            parser.reset();
        }
        else if (result.error() == Status::Waiting) {
            base::log::warning("AOF: 文件末尾有不完整的命令，已忽略");
            ++failed_count;
            break;
        }
        else {
            base::log::warning("AOF: 解析命令失败，终止重放（已执行 {} 条命令，失败 {} 条命令）",
                               succeed_count, failed_count);
            ++failed_count;
            break;
        }
    }

    application_context::replaying_aof = false;
    base::log::info("AOF: 重放完成，共执行 {} 条命令，失败 {} 条命令", succeed_count, failed_count);
}

} // namespace xin::redis
