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

#include <array>
#include <cstdlib>

using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using asio::ip::tcp;

using xin::base::log;
using xin::redis::commands;
using xin::redis::Response;
using xin::redis::RESPParser;
using arguments = RESPParser::arguments;

class Server {
public:
    using PortType = std::uint16_t;
    Server(PortType port, int thead_count = 1)
        : port_{ port }
        , context_{ thead_count }
    {
    }

    // 阻塞调用，直到服务器停止
    void start()
    {
        try {
            co_spawn(context_, listener(port_), detached);
            context_.run();
        }
        catch (const std::exception& e) {
            log::error("Server error: {}", e.what());
        }
    }

private:
    static constexpr std::size_t buffer_size = 8192;

    PortType port_;
    std::unique_ptr<tcp::acceptor> acceptor_;
    asio::io_context context_;

    auto listener(std::uint16_t port) -> awaitable<void>
    {
        acceptor_ = std::make_unique<tcp::acceptor>(co_await asio::this_coro::executor,
                                                    tcp::endpoint(tcp::v4(), port_));
        log::info("Listening on port {}", port);

        while (true) {
            auto socket = co_await acceptor_->async_accept(use_awaitable);
            // 启动一个新的协程处理这个连接，detached
            // 表示我们不关心这个协程什么时候结束（它自己会管自己）
            co_spawn(acceptor_->get_executor(), echo_session(std::move(socket)), detached);
        }
    }

    auto dispatch_command(tcp::socket socket) -> awaitable<void>
    {
        std::array<char, buffer_size> buffer{};
        std::size_t write_idx = 0;
        RESPParser parser{};
        std::vector<std::unique_ptr<Response>> responses{};

        while (true) {
            auto socket_buffer = asio::buffer(buffer.data() + write_idx, buffer.size() - write_idx);
            auto n = co_await socket.async_read_some(socket_buffer, use_awaitable);

            auto total_len = write_idx + n;
            std::span<const char> buffer_view{ buffer.data(), total_len };

            while (true) {
                auto res = parser.parse(buffer_view);

                if (res) {
                    log::debug("Command: {}", res->at(0));

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
                    break;
                }
                else {
                    co_return;
                }
            }

            if (!responses.empty()) {
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

    auto echo_session(tcp::socket socket) -> awaitable<void>
    {
        try {
            co_await dispatch_command(std::move(socket));
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
};

int main(int argc, char* argv[])
{
    constexpr std::uint16_t port = 16379;
    xin::base::log::set_level(xin::base::LogLevel::Warning);
    Server server{ port };

    try {
        server.start();
    }
    catch (const std::exception& e) {
        log::error("Server error: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
