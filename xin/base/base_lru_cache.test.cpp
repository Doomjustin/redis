#include "base_lru_cache.h"

#include <doctest/doctest.h>

#include <string>

using namespace xin::base;

TEST_SUITE("base-lru_cache")
{
    LRUCache<int, int> cache(2);

    TEST_CASE("put and get")
    {
        cache.put(1, 10);
        cache.put(2, 20);

        auto val1 = cache.get(1);
        REQUIRE(val1.has_value());
        REQUIRE(val1.value() == 10);

        auto val2 = cache.get(2);
        REQUIRE(val2.has_value());
        REQUIRE(val2.value() == 20);
    }

    TEST_CASE("get non-existent key")
    {
        auto val = cache.get(999);
        REQUIRE(!val.has_value());
    }

    TEST_CASE("eviction when capacity exceeded")
    {
        cache.put(1, 10);
        cache.put(2, 20);
        cache.put(3, 30); // 应该驱逐 1

        auto val1 = cache.get(1);
        REQUIRE(!val1.has_value());

        auto val2 = cache.get(2);
        REQUIRE(val2.has_value());
        REQUIRE(val2.value() == 20);

        auto val3 = cache.get(3);
        REQUIRE(val3.has_value());
        REQUIRE(val3.value() == 30);
    }

    TEST_CASE("access order matters")
    {
        cache.put(1, 10);
        cache.put(2, 20);

        // 访问 1，使其成为最近使用
        cache.get(1);

        // 加入 3，应该驱逐 2（最久未使用）
        cache.put(3, 30);

        auto val1 = cache.get(1);
        REQUIRE(val1.has_value());
        REQUIRE(val1.value() == 10);

        auto val2 = cache.get(2);
        REQUIRE(!val2.has_value());

        auto val3 = cache.get(3);
        REQUIRE(val3.has_value());
        REQUIRE(val3.value() == 30);
    }

    TEST_CASE("update existing key")
    {
        cache.put(1, 10);
        cache.put(2, 20);
        cache.put(1, 100); // 更新值，并将 1 移到最前

        cache.put(3, 30); // 应该驱逐 2

        auto val1 = cache.get(1);
        REQUIRE(val1.has_value());
        REQUIRE(val1.value() == 100);

        auto val2 = cache.get(2);
        REQUIRE(!val2.has_value());
    }

    TEST_CASE("string keys and values")
    {
        LRUCache<std::string, std::string> str_cache(2);
        str_cache.put("a", "alpha");
        str_cache.put("b", "beta");

        auto val = str_cache.get("a");
        REQUIRE(val.has_value());
        REQUIRE(val.value() == "alpha");

        str_cache.put("c", "gamma");
        auto val_b = str_cache.get("b");
        REQUIRE(!val_b.has_value());
    }
}
