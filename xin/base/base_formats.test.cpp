#include "base_formats.h"

#include <doctest/doctest.h>

#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace {

struct HasToString {
    [[nodiscard]]
    auto to_string() const -> std::string
    {
        return "to_string";
    }
};

struct HasToRepr {
    [[nodiscard]]
    auto to_repr() const -> std::string
    {
        return "to_repr";
    }
};

struct HasOutputOperator {
    int value{ 42 };
};

auto operator<<(std::ostream& os, const HasOutputOperator& value) -> std::ostream&
{
    os << "value=" << value.value;
    return os;
}

struct PlainType {
    int value{ 7 };
};

} // namespace

TEST_SUITE("base-xformat")
{
    TEST_CASE("xformat对to_string类型生效")
    {
        HasToString value;
        REQUIRE(xin::base::xformat("{}", value) == "to_string");
    }

    TEST_CASE("xformat在没有to_string时使用to_repr")
    {
        HasToRepr value;
        REQUIRE(xin::base::xformat("{}", value) == "to_repr");
    }

    TEST_CASE("xformat在没有to_string和to_repr时使用输出运算符")
    {
        HasOutputOperator value;
        REQUIRE(xin::base::xformat("{}", value) == "value=42");
    }

    TEST_CASE("xformat最后回退到类型名和地址")
    {
        PlainType value;
        auto text = xin::base::xformat("{}", value);
        REQUIRE(text.find("@") != std::string::npos);
    }

    TEST_CASE("xformat返回无参数格式串") { REQUIRE(xin::base::xformat("hello") == "hello"); }

    TEST_CASE("xformat支持占位符参数")
    {
        REQUIRE(xin::base::xformat("hello {} {}", "xin", 2026) == "hello xin 2026");
    }

    TEST_CASE("xformat支持vector容器输出")
    {
        std::vector<int> values{ 1, 2, 3 };
        REQUIRE(xin::base::xformat("{}", values) == "[1, 2, 3]");
    }

    TEST_CASE("xformat支持map容器输出")
    {
        std::map<std::string, int> values{ { "apple", 1 }, { "banana", 2 } };

        auto text = xin::base::xformat("{}", values);
        REQUIRE(text.find("apple") != std::string::npos);
        REQUIRE(text.find("banana") != std::string::npos);
    }
}