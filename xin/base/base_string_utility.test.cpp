#include "base_string_utility.h"

#include <doctest/doctest.h>

#include <string>

using namespace xin::base;

TEST_SUITE("base-strings")
{
    TEST_CASE("trim string 非原址修改")
    {
        std::string str1 = "   Hello, World!   ";
        auto trimmed_str1 = strings::trim(str1);
        REQUIRE(trimmed_str1 == "Hello, World!");

        std::string str2 = "\n\t  Trim me! \t\n";
        auto trimmed_str2 = strings::trim(str2);
        REQUIRE(trimmed_str2 == "Trim me!");

        std::string str3 = "NoSpaces";
        auto trimmed_str3 = strings::trim(str3);
        REQUIRE(trimmed_str3 == "NoSpaces");
    }

    TEST_CASE("left trim string 非原址修改")
    {
        std::string str1 = "   Hello, World!   ";
        auto trimmed_str1 = strings::trim_left(str1);
        REQUIRE(trimmed_str1 == "Hello, World!   ");

        std::string str2 = "\n\t  Trim me! \t\n";
        auto trimmed_str2 = strings::trim_left(str2);
        REQUIRE(trimmed_str2 == "Trim me! \t\n");

        std::string str3 = "NoSpaces";
        auto trimmed_str3 = strings::trim_left(str3);
        REQUIRE(trimmed_str3 == "NoSpaces");
    }

    TEST_CASE("right trim string 非原址修改")
    {
        std::string str1 = "   Hello, World!   ";
        auto trimmed_str1 = strings::trim_right(str1);
        REQUIRE(trimmed_str1 == "   Hello, World!");

        std::string str2 = "\n\t  Trim me! \t\n";
        auto trimmed_str2 = strings::trim_right(str2);
        REQUIRE(trimmed_str2 == "\n\t  Trim me!");

        std::string str3 = "NoSpaces";
        auto trimmed_str3 = strings::trim_right(str3);
        REQUIRE(trimmed_str3 == "NoSpaces");
    }

    TEST_CASE("split")
    {
        std::string str = "apple,banana,cherry";
        auto parts = strings::split(str, ",");
        REQUIRE(parts.size() == 3);
        REQUIRE(parts[0] == "apple");
        REQUIRE(parts[1] == "banana");
        REQUIRE(parts[2] == "cherry");
    }
}