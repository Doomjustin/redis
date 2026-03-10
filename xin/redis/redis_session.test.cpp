#include "redis_session.h"

#include <asio.hpp>
#include <asio/strand.hpp>
#include <doctest/doctest.h>
#include <redis_application_context.h>

#include <array>
#include <thread>

using asio::ip::tcp;

TEST_SUITE("redis-session")
{
    TEST_CASE("session handles ping command")
    {
        asio::io_context context{ 1 };
        asio::strand<asio::any_io_executor> strand{ context.get_executor() };
        tcp::acceptor acceptor{ context, tcp::endpoint(tcp::v4(), 0) };

        auto endpoint =
            tcp::endpoint(asio::ip::address_v4::loopback(), acceptor.local_endpoint().port());

        tcp::socket client{ context };
        client.connect(endpoint);
        tcp::socket server_socket = acceptor.accept();
        asio::co_spawn(
            context,
            [socket = std::move(server_socket), strand]() mutable -> asio::awaitable<void> {
                xin::redis::Session session{ std::move(socket), strand };

                try {
                    co_await session.run();
                }
                catch (...) {
                }
            },
            asio::detached);

        std::thread io_thread{ [&context]() { context.run(); } };

        constexpr std::string_view ping = "*1\r\n$4\r\nPING\r\n";
        asio::write(client, asio::buffer(ping.data(), ping.size()));

        std::array<char, 7> response{};
        asio::read(client, asio::buffer(response));
        CHECK(std::string{ response.data(), response.size() } == "+PONG\r\n");

        client.close();
        context.stop();
        io_thread.join();
    }
}
