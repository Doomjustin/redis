#include "redis_allocator.h"

#include <doctest/doctest.h>

#include <memory_resource>
#include <vector>

namespace {

auto aligned_size(xin::redis::allocator::SizeType size) -> xin::redis::allocator::SizeType
{
    if (size & (sizeof(long) - 1))
        size += sizeof(long) - (size & (sizeof(long) - 1));

    return size;
}

} // namespace

TEST_SUITE("redis-allocator")
{
    using xin::redis::allocator;
    using xin::redis::TrackingMemoryResource;

    TEST_CASE("malloc and free update used memory")
    {
        const auto baseline = allocator::used_memory();

        void* ptr = allocator::malloc(1);
        REQUIRE(ptr != nullptr);
        CHECK(allocator::size(ptr) == allocator::PREFIX_SIZE + aligned_size(1));
        CHECK(allocator::used_memory() == baseline + aligned_size(1));

        allocator::free(ptr);
        CHECK(allocator::used_memory() == baseline);
    }

    TEST_CASE("calloc returns zeroed memory")
    {
        const auto baseline = allocator::used_memory();

        auto* arr = allocator::calloc<int>(4);
        REQUIRE(arr != nullptr);
        CHECK(arr[0] == 0);
        CHECK(arr[1] == 0);
        CHECK(arr[2] == 0);
        CHECK(arr[3] == 0);
        CHECK(allocator::used_memory() == baseline + aligned_size(sizeof(int) * 4));

        allocator::free(arr);
        CHECK(allocator::used_memory() == baseline);
    }

    TEST_CASE("realloc grow and shrink keep accounting correct")
    {
        const auto baseline = allocator::used_memory();

        void* ptr = allocator::malloc(8);
        REQUIRE(ptr != nullptr);
        CHECK(allocator::used_memory() == baseline + aligned_size(8));

        ptr = allocator::realloc(ptr, 17);
        REQUIRE(ptr != nullptr);
        CHECK(allocator::used_memory() == baseline + aligned_size(17));

        ptr = allocator::realloc(ptr, 1);
        REQUIRE(ptr != nullptr);
        CHECK(allocator::used_memory() == baseline + aligned_size(1));

        allocator::free(ptr);
        CHECK(allocator::used_memory() == baseline);
    }

    TEST_CASE("tracking allocator updates used memory on allocate and deallocate")
    {
        TrackingMemoryResource tracking;
        CHECK(tracking.used_memory() == 0);

        void* ptr = tracking.allocate(64, alignof(std::max_align_t));
        REQUIRE(ptr != nullptr);
        CHECK(tracking.used_memory() >= 64);

        tracking.deallocate(ptr, 64, alignof(std::max_align_t));
        CHECK(tracking.used_memory() == 0);
    }

    TEST_CASE("tracking allocator works with std allocator model via pmr vector")
    {
        TrackingMemoryResource tracking;
        CHECK(tracking.used_memory() == 0);

        {
            std::pmr::polymorphic_allocator<int> alloc{ &tracking };
            std::vector<int, std::pmr::polymorphic_allocator<int>> values{ alloc };

            values.reserve(128);
            CHECK(tracking.used_memory() >= 128 * sizeof(int));

            for (int i = 0; i < 128; ++i)
                values.push_back(i);
            CHECK(values.size() == 128);
        }

        CHECK(tracking.used_memory() == 0);
    }
}
