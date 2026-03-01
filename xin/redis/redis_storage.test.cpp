#include "redis_storage.h"

#include <doctest/doctest.h>

#include <memory>
#include <vector>

TEST_SUITE("redis-storage")
{
    using namespace xin::redis;

    TEST_CASE("Database set/get 基本读写")
    {
        Database db;
        db.set("k1", std::make_shared<std::string>("v1"));

        auto got = db.get("k1");
        REQUIRE(got.has_value());
        REQUIRE(*got != nullptr);
        CHECK(**got == "v1");
    }

    TEST_CASE("Database set 同key覆盖旧值")
    {
        Database db;
        db.set("k1", std::make_shared<std::string>("v1"));
        db.set("k1", std::make_shared<std::string>("v2"));

        auto got = db.get("k1");
        REQUIRE(got.has_value());
        REQUIRE(*got != nullptr);
        CHECK(**got == "v2");
    }

    TEST_CASE("Database get 不存在key返回空")
    {
        Database db;
        auto got = db.get("missing");

        CHECK_FALSE(got.has_value());
    }

    TEST_CASE("Database erase 删除存在key并返回计数")
    {
        Database db;
        db.set("k1", std::make_shared<std::string>("v1"));
        db.set("k2", std::make_shared<std::string>("v2"));

        std::vector<Database::key_type> keys{ "k1", "k3" };
        const auto erased = db.erase(keys);

        CHECK(erased == 1);
        CHECK_FALSE(db.get("k1").has_value());
        CHECK(db.get("k2").has_value());
    }

    TEST_CASE("Database expire_at 对不存在key返回false")
    {
        Database db;
        CHECK_FALSE(db.expire_at("missing", 1));
    }

    TEST_CASE("Database key过期后get与ttl都返回空")
    {
        Database db;
        db.set("k1", std::make_shared<std::string>("v1"));

        REQUIRE(db.expire_at("k1", 0));
        CHECK_FALSE(db.get("k1").has_value());
        CHECK_FALSE(db.ttl("k1").has_value());
    }

    TEST_CASE("Database ttl 对已设置过期的key可读")
    {
        Database db;
        db.set("k1", std::make_shared<std::string>("v1"));

        REQUIRE(db.expire_at("k1", 2));
        auto remaining = db.ttl("k1");

        REQUIRE(remaining.has_value());
        CHECK(*remaining <= 2);
    }

    TEST_CASE("Database erase 对已过期key不计入删除数量")
    {
        Database db;
        db.set("k1", std::make_shared<std::string>("v1"));
        REQUIRE(db.expire_at("k1", 0));

        std::vector<Database::key_type> keys{ "k1" };
        const auto erased = db.erase(keys);

        CHECK(erased == 0);
        CHECK_FALSE(db.get("k1").has_value());
    }
}