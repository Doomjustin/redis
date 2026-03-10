#include "base_statistics.h"

#include <base_formats.h>
#include <jemalloc/jemalloc.h>

#include <new>
#include <print>

namespace xin::base {

statistics::ErrorHandler statistics::error_handler_ = nullptr;

std::atomic<std::size_t> statistics::peak_used_memory_{ 0 };

void statistics::oom_handler()
{
    if (error_handler_)
        error_handler_();
    else {
        std::println(stderr, "Out of memory - aborting");
        std::fflush(stderr);
    }
}

void statistics::on_error(ErrorHandler handler)
{
    error_handler_ = std::move(handler);
    std::set_new_handler(oom_handler);
}

auto statistics::used_memory() -> std::size_t
{
    std::uint64_t epoch = 1;
    std::size_t epoch_sz = sizeof(epoch);
    mallctl("epoch", &epoch, &epoch_sz, &epoch, epoch_sz);
    std::size_t allocated = 0;
    std::size_t sz = sizeof(allocated);
    mallctl("stats.allocated", &allocated, &sz, nullptr, 0);
    return allocated;
}

auto statistics::peak_used_memory() -> std::size_t
{
    auto current_used = used_memory();
    auto peak = peak_used_memory_.load(std::memory_order_relaxed);
    while (current_used > peak && !peak_used_memory_.compare_exchange_weak(
                                      peak, current_used, std::memory_order_relaxed)) {
    }

    return peak_used_memory_.load(std::memory_order_relaxed);
}

auto statistics::allocator_info() -> std::string
{
    const char* version;
    std::size_t size = sizeof(version);
    ::mallctl("version", &version, &size, nullptr, 0);
    return xformat("jemalloc: {}", version);
}

} // namespace xin::base
