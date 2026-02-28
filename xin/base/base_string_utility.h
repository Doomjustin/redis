#ifndef XIN_BASE_STRINGS_UTILITY_H
#define XIN_BASE_STRINGS_UTILITY_H

#include <string>
#include <string_view>
#include <vector>

namespace xin::base {

struct strings {
public:
    strings() = delete;

    static auto to_lowercase(std::string_view str) -> std::string;

    static auto to_uppercase(std::string_view str) -> std::string;

    static auto trim(std::string_view str) -> std::string;

    static auto trim_left(std::string_view str) -> std::string;

    static auto trim_right(std::string_view str) -> std::string;

    static auto split(std::string_view str, std::string_view delimeter) -> std::vector<std::string>;
};

} // namespace xin::base

#endif // XIN_BASE_STRINGS_UTILITY_H