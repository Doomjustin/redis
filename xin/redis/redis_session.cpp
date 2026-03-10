#include "redis_session.h"

#include <asio/dispatch.hpp>
#include <asio/use_awaitable.hpp>
#include <base_log.h>
#include <redis_application_context.h>
#include <redis_command.h>

using namespace xin::base;

namespace xin::redis {

Session::Session(asio::ip::tcp::socket socket, asio::strand<asio::any_io_executor> strand)
    : socket_{ std::move(socket) }
    , strand_{ std::move(strand) }
{
}

auto Session::run() -> asio::awaitable<void>
{
    while (true) {
        auto socket_buffer = asio::buffer(buffer_.data() + write_idx_, buffer_.size() - write_idx_);
        auto n = co_await socket_.async_read_some(socket_buffer, asio::use_awaitable);

        auto total_len = write_idx_ + n;
        std::span<const char> buffer_view{ buffer_.data(), total_len };

        while (true) {
            auto res = parser_.parse(buffer_view);

            if (res) {
                log::debug("Command: {}", res->at(0));

                co_await asio::dispatch(strand_, asio::use_awaitable);
                auto response = commands::dispatch(index_, *res);
                responses.push_back(std::move(response));

                parser_.reset();
                if (buffer_view.empty()) {
                    write_idx_ = 0;
                    break;
                }
            }
            else if (res.error() == Status::Waiting) {
                if (!buffer_view.empty()) {
                    std::memmove(buffer_.data(), buffer_view.data(), buffer_view.size());
                    write_idx_ = buffer_view.size();
                }
                else {
                    write_idx_ = 0;
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

            co_await asio::async_write(socket_, response_buffers, asio::use_awaitable);
            responses.clear();
        }
    }
}

} // namespace xin::redis
