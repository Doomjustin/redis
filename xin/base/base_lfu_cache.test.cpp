#include "base_lfu_cache.h"

#include <doctest/doctest.h>

#include <string>

using namespace xin::base;

TEST_SUITE("base-lfu_cache")
{
	TEST_CASE("put and get")
	{
		LFUCache<int, int> cache(2);

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
		LFUCache<int, int> cache(2);

		auto val = cache.get(999);
		REQUIRE(!val.has_value());
	}

	TEST_CASE("evict least frequently used")
	{
		LFUCache<int, int> cache(2);

		cache.put(1, 10);
		cache.put(2, 20);

		// Increase frequency of key 1 so key 2 should be evicted.
		cache.get(1);
		cache.put(3, 30);

		REQUIRE(cache.get(2) == std::nullopt);
		REQUIRE(cache.get(1).value() == 10);
		REQUIRE(cache.get(3).value() == 30);
	}

	TEST_CASE("when frequency ties evict least recently used")
	{
		LFUCache<int, int> cache(2);

		cache.put(1, 10);
		cache.put(2, 20);
		cache.put(3, 30);

		// Both 1 and 2 had same frequency; 1 is older and should be evicted.
		REQUIRE(cache.get(1) == std::nullopt);
		REQUIRE(cache.get(2).value() == 20);
		REQUIRE(cache.get(3).value() == 30);
	}

	TEST_CASE("update existing key updates value and frequency")
	{
		LFUCache<int, int> cache(2);

		cache.put(1, 10);
		cache.put(2, 20);
		cache.put(1, 100);
		cache.put(3, 30);

		REQUIRE(cache.get(2) == std::nullopt);
		REQUIRE(cache.get(1).value() == 100);
		REQUIRE(cache.get(3).value() == 30);
	}

	TEST_CASE("zero capacity cache never stores values")
	{
		LFUCache<int, int> cache(0);

		cache.put(1, 10);
		REQUIRE(cache.size() == 0);
		REQUIRE(!cache.contains(1));
		REQUIRE(cache.get(1) == std::nullopt);
	}

	TEST_CASE("string keys and values")
	{
		LFUCache<std::string, std::string> cache(2);

		cache.put("a", "alpha");
		cache.put("b", "beta");
		cache.get("a");
		cache.put("c", "gamma");

		REQUIRE(cache.get("b") == std::nullopt);
		REQUIRE(cache.get("a").value() == "alpha");
		REQUIRE(cache.get("c").value() == "gamma");
	}
}
