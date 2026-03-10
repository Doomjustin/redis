#ifndef XIN_REDIS_ALLOCATOR_H
#define XIN_REDIS_ALLOCATOR_H

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <memory_resource>

// 这里的方案都没被实际使用，因为都是依赖用户使用时手动调用，所以统计结果十分不准确。
// 特别是在C++中，用户直接使用new/delete或者别的memory resource
// 自己的代码尚可强制使用，单第三方库的代码就完全无法统计了，所以这个方案就没什么意义了。
namespace xin::redis {

// 原作API，在分配内存时会在前面预留一个size_type的空间来记录分配的大小，以便在free时正确更新used_memory_。
// 但是本作不准备用这套API了，改用一个单独的TrackingMemoryResource来实现内存跟踪
// 这里保留原API的实现以兼容现有代码，但不再推荐使用
struct allocator {
    using Size = std::size_t;
    using ErrorHandler = std::function<void(Size size)>;

    static constexpr Size PREFIX_SIZE = sizeof(Size);

    static auto malloc(Size size) -> void*;

    template<typename T>
    static auto malloc(Size size) -> T*
    {
        return static_cast<T*>(malloc(size * sizeof(T)));
    }

    static auto calloc(Size size) -> void*;

    template<typename T>
    static auto calloc(Size size) -> T*
    {
        return static_cast<T*>(calloc(size * sizeof(T)));
    }

    static auto size(void* ptr) -> Size;

    static auto realloc(void* ptr, Size size) -> void*;

    static auto free(void* ptr) -> void;

    static auto used_memory() -> Size;

    static void on_error(ErrorHandler handler);

private:
    static std::atomic<Size> used_memory_;
    static ErrorHandler error_handler_;

    static void append_used_memory(Size size);

    static void remove_used_memory(Size size);
};

// 一个基于std::pmr::memory_resource的内存跟踪器，
// 可以用来替代原来的allocator API，提供更现代的接口和更准确的内存使用统计
class TrackingMemoryResource : public std::pmr::memory_resource {
public:
    using ErrorHandler = std::function<void(std::size_t size)>;

    explicit TrackingMemoryResource(
        std::pmr::memory_resource* upstream = std::pmr::get_default_resource());

    [[nodiscard]]
    constexpr auto used_memory() const noexcept -> std::size_t
    {
        return total_allocated_.load(std::memory_order_relaxed);
    }

    void on_error(ErrorHandler handler) { error_handler_ = std::move(handler); }

private:
    std::pmr::memory_resource* upstream_;
    std::atomic<std::size_t> total_allocated_{ 0 };
    ErrorHandler error_handler_;

    auto do_allocate(std::size_t bytes, std::size_t alignment) -> void* override;

    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override;

    [[nodiscard]]
    constexpr auto do_is_equal(const std::pmr::memory_resource& other) const noexcept
        -> bool override
    {
        return this == &other;
    }
};

} // namespace xin::redis

#endif // XIN_REDIS_ALLOCATOR_H
