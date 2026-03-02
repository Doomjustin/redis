#include "redis_command_sorted_set.h"

#include <doctest/doctest.h>

TEST_SUITE("redis-command-sorted-set")
{
    TEST_CASE("sorted set commands placeholder compiles")
    {
        CHECK(sizeof(xin::redis::sorted_set_commands) > 0);
    }
}