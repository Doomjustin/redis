#include "redis_command_define.h"

#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/use_awaitable.hpp>
#include <base_log.h>
#include <redis_command.h>
#include <redis_response.h>
#include <redis_serialization_protocl.h>

#include <algorithm>
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
        constexpr std::size_t buffer_size = 8192;

        std::array<char, buffer_size> buffer{};
        RESPParser parser{};
        std::size_t write_idx = 0;
        std::vector<ResponsePtr> responses{};

        while (true) {
            auto socket_buffer = asio::buffer(buffer.data() + write_idx, buffer.size() - write_idx);
            auto n = co_await socket.async_read_some(socket_buffer, use_awaitable);

            auto total_len = write_idx + n;
            std::span<const char> buffer_view{ buffer.data(), total_len };

            bool has_parsed_command = false;
            // 解析 RESP 协议
            while (true) {
                auto res = parser.parse(buffer_view);

                if (res) {
                    log::info("Command: {}", res->at(0));
                    has_parsed_command = true;

                    auto response = commands::dispatch(*res);
                    responses.push_back(std::move(response));

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

            if (has_parsed_command && !responses.empty()) {
                std::vector<asio::const_buffer> response_buffers{};
                for (const auto& res : responses) {
                    auto buffers = res->to_buffer();
                    response_buffers.insert(response_buffers.end(), buffers.begin(), buffers.end());
                }

                co_await asio::async_write(socket, response_buffers, use_awaitable);
                responses.clear();
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
