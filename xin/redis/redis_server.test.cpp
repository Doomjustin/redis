#include "redis_server.h"

#include <doctest/doctest.h>

#include <type_traits>

TEST_SUITE("redis-server")
{
    TEST_CASE("server type contract")
    {
        using xin::redis::Server;

        CHECK(std::is_constructible_v<Server, Server::PortType>);
        CHECK(!std::is_copy_constructible_v<Server>);
        CHECK(!std::is_copy_assignable_v<Server>);
        CHECK(!std::is_move_constructible_v<Server>);
        CHECK(!std::is_move_assignable_v<Server>);
    }
}
