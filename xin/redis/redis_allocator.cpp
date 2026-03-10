#include "redis_allocator.h"

#include <malloc.h>

#include <atomic>
#include <cstdlib>
#include <new>
#include <print>

namespace xin::redis {

void default_oom_handler(allocator::SizeType size)
{
    std::println(stderr, "Out of memory trying to allocate {} bytes", size);
    std::fflush(stderr);
    std::abort();
}

auto assign(allocator::SizeType size) -> allocator::SizeType
{
    if (size & (sizeof(long) - 1))
        size += sizeof(long) - (size & (sizeof(long) - 1));

    return size;
}

std::atomic<allocator::SizeType> allocator::used_memory_{ 0 };

allocator::ErrorHandler allocator::error_handler_ = default_oom_handler;

auto allocator::malloc(SizeType size) -> void*
{
    auto* ptr = std::malloc(size + PREFIX_SIZE);
    if (!ptr)
        error_handler_(size);

    *static_cast<SizeType*>(ptr) = size;
    append_used_memory(size);
    return static_cast<char*>(ptr) + PREFIX_SIZE;
}

auto allocator::calloc(SizeType size) -> void*
{
    auto* ptr = std::calloc(1, size + PREFIX_SIZE);
    if (!ptr)
        error_handler_(size);

    *static_cast<SizeType*>(ptr) = size;
    append_used_memory(size);
    return static_cast<char*>(ptr) + PREFIX_SIZE;
}

auto allocator::size(void* ptr) -> SizeType
{
    auto* real_ptr = static_cast<char*>(ptr) - PREFIX_SIZE;
    auto size = *reinterpret_cast<SizeType*>(real_ptr);
    return assign(size) + PREFIX_SIZE;
}

auto allocator::realloc(void* ptr, SizeType size) -> void*
{
    if (!ptr)
        return malloc(size);

    auto* real_ptr = static_cast<char*>(ptr) - PREFIX_SIZE;
    auto old_size = *reinterpret_cast<SizeType*>(real_ptr);
    auto* new_ptr = std::realloc(real_ptr, size + PREFIX_SIZE);
    if (!new_ptr)
        error_handler_(size);

    *static_cast<SizeType*>(new_ptr) = size;
    remove_used_memory(old_size);
    append_used_memory(size);
    return static_cast<char*>(new_ptr) + PREFIX_SIZE;
}

void allocator::free(void* ptr)
{
    if (!ptr)
        return;

    auto* real_ptr = static_cast<char*>(ptr) - PREFIX_SIZE;
    auto old_size = *reinterpret_cast<SizeType*>(real_ptr);
    remove_used_memory(old_size);
    std::free(real_ptr);
}

auto allocator::used_memory() -> SizeType { return used_memory_.load(std::memory_order_relaxed); }

void allocator::on_error(ErrorHandler handler) { error_handler_ = std::move(handler); }

void allocator::append_used_memory(SizeType size)
{
    used_memory_.fetch_add(assign(size), std::memory_order_relaxed);
}

void allocator::remove_used_memory(SizeType size)
{
    used_memory_.fetch_sub(assign(size), std::memory_order_relaxed);
}

TrackingMemoryResource::TrackingMemoryResource(std::pmr::memory_resource* upstream)
    : upstream_{ upstream }
    , error_handler_{ default_oom_handler }
{
}

auto TrackingMemoryResource::do_allocate(std::size_t bytes, std::size_t alignment) -> void*
{
    void* ptr;

    try {
        ptr = upstream_->allocate(bytes, alignment);
    }
    catch (std::bad_alloc& e) {
        error_handler_(bytes);
    }

    auto actual_size = malloc_usable_size(ptr);
    total_allocated_.fetch_add(actual_size, std::memory_order_relaxed);
    return ptr;
}

void TrackingMemoryResource::do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment)
{
    auto actual_size = malloc_usable_size(ptr);
    upstream_->deallocate(ptr, bytes, alignment);
    total_allocated_.fetch_sub(actual_size, std::memory_order_relaxed);
}

} // namespace xin::redis
