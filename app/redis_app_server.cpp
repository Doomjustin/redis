#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/use_awaitable.hpp>
#include <base_log.h>
#include <redis_command.h>
#include <redis_response.h>
#include <redis_serialization_protocl.h>

#include <array>
#include <cstdlib>

using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using asio::ip::tcp;

using xin::base::log;
using xin::redis::RESPParser;
using arguments = RESPParser::arguments;

auto echo_session(tcp::socket socket) -> awaitable<void>
{
    using namespace xin::redis;
    try {
        std::array<char, 1024> buffer{};
        RESPParser parser{};
        std::size_t write_idx = 0;

        while (true) {
            auto socket_buffer = asio::buffer(buffer.data() + write_idx, buffer.size() - write_idx);
            auto n = co_await socket.async_read_some(socket_buffer, use_awaitable);

            auto total_len = write_idx + n;
            std::span<const char> buffer_view{ buffer.data(), total_len };

            // 解析 RESP 协议
            while (true) {
                auto res = parser.parse(buffer_view);

                if (res) {
                    log::info("Command: {}", res->at(0));

                    // WARNING:
                    // 这个算是一个坑，async_write不能传给他一个临时对象buffer，必须给他一个有效的对象
                    auto response = commands::dispatch(*res);
                    auto buffers = response->to_buffer();
                    co_await asio::async_write(socket, buffers, use_awaitable);

                    parser.reset();
                    if (buffer_view.empty()) {
                        write_idx = 0;
                        break;
                    }
                }
                else if (res.error() == RESPParser::Error::Waiting) {
                    if (!buffer_view.empty()) {
                        std::memmove(buffer.data(), buffer_view.data(), buffer_view.size());
                        write_idx = buffer_view.size();
                    }
                    else {
                        write_idx = 0;
                    }
                }
                else {
                    co_return;
                }
            }
        }
    }
    catch (const asio::system_error& e) {
        if (e.code() == asio::error::eof)
            log::info("Client disconnected");
        else
            log::error("Session error: {}", e.what());
    }
    catch (const std::exception& e) {
        log::error("Exception in Session: {}", e.what());
    }
}

auto listener(std::uint16_t port) -> awaitable<void>
{
    tcp::acceptor acceptor(co_await asio::this_coro::executor, tcp::endpoint(tcp::v4(), port));
    log::info("Listening on port {}", port);

    while (true) {
        auto socket = co_await acceptor.async_accept(use_awaitable);
        // 启动一个新的协程处理这个连接，detached
        // 表示我们不关心这个协程什么时候结束（它自己会管自己）
        co_spawn(acceptor.get_executor(), echo_session(std::move(socket)), detached);
    }
}

int main(int argc, char* argv[])
{
    constexpr std::uint16_t port = 16379;
    xin::base::log::set_level(xin::base::LogLevel::Warning);

    try {
        asio::io_context ctx(1);
        co_spawn(ctx, listener(port), detached);
        ctx.run();
    }
    catch (const std::exception& e) {
        log::error("Server error: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
