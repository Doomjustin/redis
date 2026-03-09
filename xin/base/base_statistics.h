#ifndef XIN_BASE_STATISTICS_H
#define XIN_BASE_STATISTICS_H

#include <atomic>
#include <cstddef>
#include <functional>
#include <string>

namespace xin::base {

struct statistics {
    using error_handler = std::function<void()>;

    statistics() = delete;

    static void on_error(error_handler handler);

    static auto used_memory() -> std::size_t;

    static auto peak_used_memory() -> std::size_t;

    static auto allocator_info() -> std::string;

private:
    static error_handler error_handler_;
    static std::atomic<std::size_t> peak_used_memory_;

    static void oom_handler();
};

} // namespace xin::base

#endif // XIN_BASE_STATISTICS_H
