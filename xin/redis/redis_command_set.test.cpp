#include "redis_command_set.h"

#include <doctest/doctest.h>

TEST_SUITE("redis-command-set")
{
    TEST_CASE("set commands placeholder compiles") { CHECK(sizeof(xin::redis::set_commands) > 0); }
}