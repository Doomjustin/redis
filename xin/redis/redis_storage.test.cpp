#include "redis_storage.h"

#include <doctest/doctest.h>

#include <memory>
#include <unordered_map>
#include <vector>

TEST_SUITE("redis-storage")
{
    using namespace xin::redis;
    using string_type = Database::StringPtr;
    using hash_type = Database::HashPtr;

    TEST_CASE("Database set/get_if<string_type> 基本读写")
    {
        Database db;
        db.set("k1", std::make_shared<std::string>("v1"));

        auto got = db.get_if<string_type>("k1");
        REQUIRE(got.has_value());
        CHECK(**got == "v1");
    }

    TEST_CASE("Database set 同key覆盖旧值")
    {
        Database db;
        db.set("k1", std::make_shared<std::string>("v1"));
        db.set("k1", std::make_shared<std::string>("v2"));

        auto got = db.get_if<string_type>("k1");
        REQUIRE(got.has_value());
        CHECK(**got == "v2");
    }

    TEST_CASE("Database get 不存在key返回空")
    {
        Database db;
        auto got = db.get("missing");

        CHECK_FALSE(got.has_value());
    }

    TEST_CASE("Database get_if 类型不匹配时返回空")
    {
        Database db;
        auto h = std::make_shared<std::unordered_map<Database::KeyType, string_type>>();
        h->insert_or_assign("field", std::make_shared<std::string>("value"));
        db.set("hk", h);

        auto as_string = db.get_if<string_type>("hk");
        auto as_hash = db.get_if<hash_type>("hk");

        CHECK_FALSE(as_string.has_value());
        REQUIRE(as_hash.has_value());
        CHECK((*as_hash)->contains("field"));
    }

    TEST_CASE("Database mget 保序并对未命中返回nullptr")
    {
        Database db;
        db.set("k1", std::make_shared<std::string>("v1"));
        db.set("k3", std::make_shared<std::string>("v3"));

        std::vector<Database::KeyType> keys{ "k1", "k2", "k3" };
        auto got = db.mget(keys);

        REQUIRE(got.size() == 3);
        REQUIRE(got[0] != nullptr);
        CHECK(*got[0] == "v1");
        CHECK(got[1] == nullptr);
        REQUIRE(got[2] != nullptr);
        CHECK(*got[2] == "v3");
    }

    TEST_CASE("Database erase 删除存在key并返回计数")
    {
        Database db;
        db.set("k1", std::make_shared<std::string>("v1"));
        db.set("k2", std::make_shared<std::string>("v2"));

        std::vector<Database::KeyType> keys{ "k1", "k3" };
        const auto erased = db.erase(keys);

        CHECK(erased == 1);
        CHECK_FALSE(db.get_if<string_type>("k1").has_value());
        CHECK(db.get_if<string_type>("k2").has_value());
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
        CHECK_FALSE(db.get_if<string_type>("k1").has_value());
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

        std::vector<Database::KeyType> keys{ "k1" };
        const auto erased = db.erase(keys);

        CHECK(erased == 0);
        CHECK_FALSE(db.get_if<string_type>("k1").has_value());
    }
}