#include "redis_response.h"

#include <base_formats.h>

#include <string_view>

namespace xin::redis {

using namespace xin::base;

ErrorResponse::ErrorResponse(std::string_view err)
    : content_{ xformat("-{}\r\n", err) }
{
}

[[nodiscard]]
auto ErrorResponse::to_buffer() const -> buffers
{
    buffers result{};
    result.emplace_back(content_.data(), content_.size());
    return result;
}

SimpleStringResponse::SimpleStringResponse(std::string_view content)
    : content_{ xformat("+{}\r\n", content) }
{
}

auto SimpleStringResponse::to_buffer() const -> buffers
{
    buffers result{};
    result.emplace_back(content_.data(), content_.size());
    return result;
}

IntegralResponse::IntegralResponse(long num)
    : content_{ xformat(":{}\r\n", num) }
{
}

auto IntegralResponse::to_buffer() const -> buffers
{
    buffers result{};
    result.emplace_back(content_.data(), content_.size());
    return result;
}

void BulkStringResponse::add_content(std::shared_ptr<std::string> content)
{
    contents_.emplace_back(std::move(content));
    header_ = xformat("${}\r\n", contents_.back()->size());
}

auto BulkStringResponse::to_buffer() const -> buffers
{
    buffers result{};
    result.emplace_back(header_.data(), header_.size());

    for (const auto& content : contents_)
        result.emplace_back(content->data(), content->size());

    constexpr std::string_view footer = "\r\n";
    result.emplace_back(footer.data(), footer.size());

    return result;
}

auto NullBulkStringResponse::to_buffer() const -> buffers
{
    constexpr std::string_view null_bulk_str = "$-1\r\n";

    buffers result{};
    result.emplace_back(null_bulk_str.data(), null_bulk_str.size());
    return result;
}

} // namespace xin::redis