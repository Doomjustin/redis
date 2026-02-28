#include "base_string_utility.h"

#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view WHITESPACE = " \t\n\r";

} // namespace

namespace xin::base {

auto strings::to_lowercase(std::string_view str) -> std::string
{
    auto to_lower_char = [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    };

    auto view = str | std::views::transform(to_lower_char);
    return { view.begin(), view.end() };
}

auto strings::to_uppercase(std::string_view str) -> std::string
{
    auto to_upper_char = [](char c) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    };

    auto view = str | std::views::transform(to_upper_char);
    return { view.begin(), view.end() };
}

auto strings::trim(std::string_view str) -> std::string
{
    auto from = str.find_first_not_of(WHITESPACE);
    if (from == std::string_view::npos)
        return {};

    auto to = str.find_last_not_of(WHITESPACE);
    return std::string{ str.substr(from, to - from + 1) };
}

auto strings::trim_left(std::string_view str) -> std::string
{
    auto from = str.find_first_not_of(WHITESPACE);
    if (from == std::string_view::npos)
        return {};

    return std::string{ str.substr(from) };
}

auto strings::trim_right(std::string_view str) -> std::string
{
    auto to = str.find_last_not_of(WHITESPACE);
    if (to == std::string_view::npos)
        return {};

    return std::string{ str.substr(0, to + 1) };
}

auto strings::split(std::string_view str, std::string_view delimeter) -> std::vector<std::string>
{
    auto to_string = [](auto&& rng) { return std::string{ rng.begin(), rng.end() }; };

    auto view = str | std::views::split(delimeter) | std::views::transform(to_string);

    return { view.begin(), view.end() };
}

} // namespace xin::base