#include "redis_response.h"

#include <base_formats.h>

#include <string_view>

namespace {

constexpr std::string_view footer = "\r\n";

} // namespace

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

SingleBulkStringResponse::SingleBulkStringResponse(std::shared_ptr<std::string> content)
    : content_{ std::move(content) }
{
    size_header_ = xformat("${}\r\n", content_->size());
}

auto SingleBulkStringResponse::to_buffer() const -> buffers
{
    buffers result{};
    result.emplace_back(size_header_.data(), size_header_.size());
    result.emplace_back(content_->data(), content_->size());
    result.emplace_back(footer.data(), footer.size());
    return result;
}

void BulkStringResponse::add_record(std::shared_ptr<std::string> content)
{
    const auto content_size = content->size();
    contents_.emplace_back(std::move(content));
    content_size_.emplace_back(xformat("${}\r\n", content_size));
    header_ = xformat("*{}\r\n", contents_.size());
}

auto BulkStringResponse::to_buffer() const -> buffers
{
    buffers result{};
    result.emplace_back(header_.data(), header_.size());

    for (size_t i = 0; i < contents_.size(); ++i) {
        result.emplace_back(content_size_[i].data(), content_size_[i].size());
        result.emplace_back(contents_[i]->data(), contents_[i]->size());
        result.emplace_back(footer.data(), footer.size());
    }

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