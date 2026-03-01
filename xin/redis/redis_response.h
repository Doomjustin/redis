#ifndef XIN_REDIS_RESPONSE_H
#define XIN_REDIS_RESPONSE_H

#include <asio.hpp>

#include <string_view>
#include <vector>

namespace xin::redis {

using buffers = std::vector<asio::const_buffer>;

struct Response {
    virtual ~Response() = default;

    [[nodiscard]]
    virtual auto to_buffer() const -> buffers = 0;
};

class ErrorResponse : public Response {
public:
    ErrorResponse(std::string_view err);
    [[nodiscard]]
    auto to_buffer() const -> buffers override;

private:
    std::string content_;
};

class SimpleStringResponse : public Response {
public:
    SimpleStringResponse(std::string_view content);

    [[nodiscard]]
    auto to_buffer() const -> buffers override;

private:
    std::string content_;
};

class IntegralResponse : public Response {
public:
    IntegralResponse(long num);

    [[nodiscard]]
    auto to_buffer() const -> buffers override;

private:
    std::string content_;
};

class SingleBulkStringResponse : public Response {
public:
    SingleBulkStringResponse(std::shared_ptr<std::string> content);

    [[nodiscard]]
    auto to_buffer() const -> buffers override;

private:
    std::string size_header_ = "$1\r\n";
    std::shared_ptr<std::string> content_;
};

class BulkStringResponse : public Response {
public:
    // 通过shared_ptr来避免多次复制字符串内容
    void add_record(std::shared_ptr<std::string> content);

    [[nodiscard]]
    auto to_buffer() const -> buffers override;

private:
    std::string header_ = "*0\r\n";
    std::vector<std::string> content_size_;
    std::vector<std::shared_ptr<std::string>> contents_;
};

class NullBulkStringResponse : public Response {
public:
    [[nodiscard]]
    auto to_buffer() const -> buffers override;
};

} // namespace xin::redis

#endif // XIN_REDIS_RESPONSE_H