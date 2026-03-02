#include "redis_sorted_set.h"

#include <doctest/doctest.h>

TEST_SUITE("redis-sorted-set")
{
    using xin::redis::SortedSet;

    TEST_CASE("insert orders by score then member")
    {
        SortedSet set;

        CHECK(set.insert_or_assign(2.0, std::make_shared<std::string>("two")));
        CHECK(set.insert_or_assign(1.0, std::make_shared<std::string>("one")));
        CHECK(set.insert_or_assign(2.0, std::make_shared<std::string>("abc")));

        REQUIRE(set.size() == 3);

        auto it = set.begin();
        REQUIRE(it != set.end());
        CHECK(*it->member == "one");
        CHECK(it->score == doctest::Approx(1.0));

        ++it;
        REQUIRE(it != set.end());
        CHECK(*it->member == "abc");
        CHECK(it->score == doctest::Approx(2.0));

        ++it;
        REQUIRE(it != set.end());
        CHECK(*it->member == "two");
        CHECK(it->score == doctest::Approx(2.0));
    }

    TEST_CASE("insert_or_assign updates score and returns false for existing member")
    {
        SortedSet set;

        CHECK(set.insert_or_assign(1.0, std::make_shared<std::string>("one")));
        CHECK_FALSE(set.insert_or_assign(2.0, std::make_shared<std::string>("one")));

        REQUIRE(set.size() == 1);
        CHECK(set.begin()->score == doctest::Approx(2.0));
        CHECK(*set.begin()->member == "one");
    }

    TEST_CASE("insert_or_assign with same score and same member returns false")
    {
        SortedSet set;

        CHECK(set.insert_or_assign(1.0, std::make_shared<std::string>("one")));
        CHECK_FALSE(set.insert_or_assign(1.0, std::make_shared<std::string>("one")));
        CHECK(set.size() == 1);
    }
}