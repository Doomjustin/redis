#include "base_aof_logger.h"

#include <cerrno>
#include <fstream>
#include <mutex>

namespace xin::base {

AOFLogger::AOFLogger(std::filesystem::path file_path)
    : file_path_{ std::move(file_path) }
    , flush_thread_{ &AOFLogger::flush_loop, this }
{
    front_buffer_.reserve(BUFFER_MAX_SIZE);
    back_buffer_.reserve(BUFFER_MAX_SIZE);
}

AOFLogger::~AOFLogger()
{
    {
        std::lock_guard<std::mutex> locker{ m_ };
        stop_ = true;
    }

    cv_.notify_all();

    if (flush_thread_.joinable())
        flush_thread_.join();
}

auto AOFLogger::append(std::string_view command) -> std::expected<void, RejectReason>
{
    if (command.size() > BUFFER_REJECT_SIZE)
        return std::unexpected{ RejectReason::Backpressure };

    bool should_notify = false;

    {
        std::unique_lock<std::mutex> locker{ m_ };

        if (stop_)
            return std::unexpected{ RejectReason::Stopped };

        if (last_error_)
            return std::unexpected{ RejectReason::IOError };

        auto has_space = [this, &command]() {
            return front_buffer_.size() + command.size() <= BUFFER_REJECT_SIZE;
        };

        if (!has_space()) {
            cv_.wait_for(locker, APPEND_WAIT_TIMEOUT, [this, &has_space]() {
                return stop_ || last_error_.has_value() || has_space();
            });

            if (stop_)
                return std::unexpected{ RejectReason::Stopped };

            if (last_error_)
                return std::unexpected{ RejectReason::IOError };

            if (!has_space())
                return std::unexpected{ RejectReason::Backpressure };
        }

        front_buffer_ += command;
        should_notify = front_buffer_.size() >= BUFFER_MAX_SIZE;
    }

    if (should_notify)
        cv_.notify_one();

    return {};
}

auto AOFLogger::last_error() const -> std::optional<AofError>
{
    std::lock_guard<std::mutex> locker{ m_ };
    return last_error_;
}

void AOFLogger::flush_loop()
{
    std::fstream file{ file_path_, std::ios::out | std::ios::app };

    if (!file.is_open()) {
        set_io_error(std::error_code{ errno, std::generic_category() }, "failed to open aof file");
        return;
    }

    while (true) {
        auto awake = [this]() { return stop_ || !front_buffer_.empty(); };

        {
            std::unique_lock<std::mutex> locker{ m_ };
            cv_.wait_for(locker, FLUSH_INTERVAL, awake);

            if (stop_ && front_buffer_.empty())
                break;

            std::swap(front_buffer_, back_buffer_);
        }

        // Wake up blocked appenders after making room in front buffer.
        cv_.notify_all();

        if (!back_buffer_.empty()) {
            file.write(back_buffer_.data(), back_buffer_.size());
            file.flush();

            if (!file.good()) {
                set_io_error(std::error_code{ errno, std::generic_category() },
                             "failed to write or flush aof file");
                break;
            }

            back_buffer_.clear();
        }
    }
}

void AOFLogger::set_io_error(std::error_code code, std::string message)
{
    std::lock_guard<std::mutex> locker{ m_ };

    const auto next_count = last_error_.has_value() ? (last_error_->count + 1) : 1;
    last_error_ = AofError{ .code = code,
                            .message = std::move(message),
                            .timestamp = std::chrono::system_clock::now(),
                            .count = next_count };

    cv_.notify_all();
}

} // namespace xin::base
