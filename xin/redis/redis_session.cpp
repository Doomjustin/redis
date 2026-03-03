#include "redis_session.h"

#include <base_log.h>
#include <redis_command.h>

using namespace asio;
using namespace xin::base;

namespace xin::redis {

Session::Session(asio::ip::tcp::socket socket)
    : socket_{ std::move(socket) }
{
}

auto Session::start() -> asio::awaitable<void>
{
    while (true) {
        auto socket_buffer = asio::buffer(buffer_.data() + write_idx_, buffer_.size() - write_idx_);
        auto n = co_await socket_.async_read_some(socket_buffer, use_awaitable);

        auto total_len = write_idx_ + n;
        std::span<const char> buffer_view{ buffer_.data(), total_len };

        while (true) {
            auto res = parser_.parse(buffer_view);

            if (res) {
                log::debug("Command: {}", res->at(0));

                auto response = commands::dispatch(*res);
                responses.push_back(std::move(response));

                parser_.reset();
                if (buffer_view.empty()) {
                    write_idx_ = 0;
                    break;
                }
            }
            else if (res.error() == RESPParser::Error::Waiting) {
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

            co_await asio::async_write(socket_, response_buffers, use_awaitable);
            responses.clear();
        }
    }
}

} // namespace xin::redis
