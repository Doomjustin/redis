#ifndef XIN_BASE_AOF_LOGGER_H
#define XIN_BASE_AOF_LOGGER_H

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

namespace xin::base {

enum class RejectReason : std::uint8_t {
    Stopped,
    IOError,
    Backpressure,
};

struct AofError {
    std::error_code code;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::uint64_t count = 0;
};

class AOFLogger {
public:
    explicit AOFLogger(std::filesystem::path file_path);

    AOFLogger(const AOFLogger&) = delete;
    auto operator=(const AOFLogger&) -> AOFLogger& = delete;

    AOFLogger(AOFLogger&&) = delete;
    auto operator=(AOFLogger&&) -> AOFLogger& = delete;

    ~AOFLogger();

    auto append(std::string_view command) -> std::expected<void, RejectReason>;

    auto last_error() const -> std::optional<AofError>;

private:
    static constexpr std::size_t BUFFER_MAX_SIZE = 4 * 1024 * 1024;    // 4MB
    static constexpr std::size_t BUFFER_REJECT_SIZE = 8 * 1024 * 1024; // 8MB
    static constexpr std::chrono::milliseconds APPEND_WAIT_TIMEOUT =
        std::chrono::milliseconds{ 20 };
    static constexpr std::chrono::seconds FLUSH_INTERVAL = std::chrono::seconds{ 1 };

    std::filesystem::path file_path_;
    std::string front_buffer_;
    std::string back_buffer_;

    mutable std::mutex m_;
    std::condition_variable cv_;
    bool stop_ = false;
    bool io_error_ = false;
    std::optional<AofError> last_error_;
    std::thread flush_thread_;

    void flush_loop();

    void set_io_error(std::error_code code, std::string message);
};

} // namespace xin::base

#endif // XIN_BASE_AOF_LOGGER_H
