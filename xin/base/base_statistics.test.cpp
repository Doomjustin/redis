#include "base_statistics.h"

#include <doctest/doctest.h>

#include <cstdlib>
#include <limits>
#include <new>

TEST_SUITE("redis.statics")
{
    using namespace xin::base;
    // -----------------------------------------------------------------------
    // All tests measure deltas against a baseline taken immediately before
    // each allocation so framework/background allocations do not interfere.
    // -----------------------------------------------------------------------

    TEST_CASE("malloc increases used_memory by at least requested size")
    {
        constexpr std::size_t alloc_size = 65536;
        const auto before = statistics::used_memory();
        void* p = std::malloc(alloc_size);
        REQUIRE(p != nullptr);
        CHECK(statistics::used_memory() - before >= alloc_size);
        std::free(p);
    }

    TEST_CASE("free decreases used_memory")
    {
        constexpr std::size_t alloc_size = 65536;
        void* p = std::malloc(alloc_size);
        REQUIRE(p != nullptr);
        const auto after_alloc = statistics::used_memory();
        std::free(p);
        CHECK(statistics::used_memory() < after_alloc);
    }

    TEST_CASE("calloc increases used_memory by at least nmemb*size bytes")
    {
        constexpr std::size_t nmemb = 1024;
        constexpr std::size_t elem = 64;
        const auto before = statistics::used_memory();
        void* p = std::calloc(nmemb, elem);
        REQUIRE(p != nullptr);
        CHECK(statistics::used_memory() - before >= nmemb * elem);
        std::free(p);
    }

    TEST_CASE("realloc to larger size increases used_memory")
    {
        void* p = std::malloc(64);
        REQUIRE(p != nullptr);
        const auto after_small = statistics::used_memory();
        void* np = std::realloc(p, 65536);
        REQUIRE(np != nullptr);
        CHECK(statistics::used_memory() > after_small);
        std::free(np);
    }

    TEST_CASE("realloc to smaller size decreases used_memory")
    {
        void* p = std::malloc(65536);
        REQUIRE(p != nullptr);
        const auto after_large = statistics::used_memory();
        void* np = std::realloc(p, 64);
        REQUIRE(np != nullptr);
        CHECK(statistics::used_memory() < after_large);
        std::free(np);
    }

    TEST_CASE("realloc(ptr, 0) subtracts old size")
    {
        void* p = std::malloc(65536);
        REQUIRE(p != nullptr);
        const auto after_alloc = statistics::used_memory();
        // realloc(ptr, 0) may free the block; counter must go down.
        void* np = std::realloc(p, 0);
        CHECK(statistics::used_memory() < after_alloc);
        if (np)
            std::free(np);
    }

    TEST_CASE("peak_used_memory is at least current usage after large alloc")
    {
        constexpr std::size_t big = 2 * 1024 * 1024; // 2 MiB
        void* p = std::malloc(big);
        REQUIRE(p != nullptr);
        const auto current = statistics::used_memory();
        const auto peak = statistics::peak_used_memory();
        CHECK(peak >= current);
        std::free(p);
    }

    TEST_CASE("peak_used_memory does not decrease after free")
    {
        void* p = std::malloc(2 * 1024 * 1024);
        REQUIRE(p != nullptr);
        // Capture peak while memory is still held.
        const auto peak_before_free = statistics::peak_used_memory();
        std::free(p);
        CHECK(statistics::peak_used_memory() >= peak_before_free);
    }

    TEST_CASE("set_oom_handler is called when operator new fails")
    {
        bool handler_called = false;

        statistics::on_error([&handler_called] {
            handler_called = true;
            // Unregister ourselves so the next new attempt throws bad_alloc
            // instead of looping forever.
            std::set_new_handler(nullptr);
        });

        // nothrow new: returns nullptr without invoking the handler.
        // Throwing new: invokes the handler once, then throws bad_alloc.
        CHECK_THROWS_AS(::operator new(std::numeric_limits<std::size_t>::max()), std::bad_alloc);
        CHECK(handler_called);

        // Restore: no handler (runtime will throw bad_alloc directly).
        statistics::on_error(nullptr);
    }
}
