#include "redis_response.h"

#include <doctest/doctest.h>

#include <memory>
#include <string>

namespace {

auto buffers_to_string(const xin::redis::buffers& bs) -> std::string
{
    std::string out;
    for (const auto& b : bs)
        out.append(static_cast<const char*>(b.data()), b.size());

    return out;
}

} // namespace

TEST_SUITE("redis-response")
{
    using namespace xin::redis;

    TEST_CASE("ErrorResponse 序列化")
    {
        ErrorResponse resp{ "ERR wrong" };
        CHECK(buffers_to_string(resp.to_buffer()) == "-ERR wrong\r\n");
    }

    TEST_CASE("SimpleStringResponse 序列化")
    {
        SimpleStringResponse resp{ "OK" };
        CHECK(buffers_to_string(resp.to_buffer()) == "+OK\r\n");
    }

    TEST_CASE("IntegralResponse 序列化")
    {
        IntegralResponse resp{ 42 };
        CHECK(buffers_to_string(resp.to_buffer()) == ":42\r\n");
    }

    TEST_CASE("SingleBulkStringResponse 单内容序列化")
    {
        SingleBulkStringResponse resp{ std::make_shared<std::string>("hello") };
        CHECK(buffers_to_string(resp.to_buffer()) == "$5\r\nhello\r\n");
    }

    TEST_CASE("BulkStringResponse 单记录按数组序列化")
    {
        BulkStringResponse resp;
        resp.add_record(std::make_shared<std::string>("hello"));
        CHECK(buffers_to_string(resp.to_buffer()) == "*1\r\n$5\r\nhello\r\n");
    }

    TEST_CASE("BulkStringResponse 空记录序列化为空数组")
    {
        BulkStringResponse resp;
        CHECK(buffers_to_string(resp.to_buffer()) == "*0\r\n");
    }

    TEST_CASE("BulkStringResponse 多记录按标准RESP数组序列化")
    {
        BulkStringResponse resp;
        resp.add_record(std::make_shared<std::string>("hello"));
        resp.add_record(std::make_shared<std::string>("world"));

        CHECK(buffers_to_string(resp.to_buffer()) == "*2\r\n$5\r\nhello\r\n$5\r\nworld\r\n");
    }

    TEST_CASE("NullBulkStringResponse 序列化")
    {
        NullBulkStringResponse resp;
        CHECK(buffers_to_string(resp.to_buffer()) == "$-1\r\n");
    }
}