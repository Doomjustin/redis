#ifndef XIN_REDIS_STORAGE_H
#define XIN_REDIS_STORAGE_H

#include <chrono>
#include <concepts>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>

namespace xin::redis {

class Database {
public:
    using key_type = std::string;
    using value_type = std::shared_ptr<std::string>;
    using clock = std::chrono::steady_clock;
    using time_point = clock::time_point;
    using duration = clock::duration;
    using time_unit = std::chrono::milliseconds;
    using time_t = std::uint64_t;
    using size_type = std::size_t;

    void set(key_type key, value_type value);

    void set(key_type key, value_type value, time_t expire_seconds);

    auto get(const key_type& key) -> std::optional<value_type>;

    template<typename Range>
        requires std::ranges::input_range<Range> &&
                 std::same_as<std::ranges::range_value_t<Range>, key_type>
    auto get(Range&& keys) -> std::vector<value_type>
    {
        std::vector<value_type> result{};
        for (const auto& key : keys) {
            auto res = get(key);
            if (res)
                result.emplace_back(*res);
            else
                result.emplace_back(nullptr);
        }
        return result;
    }

    template<typename Range>
        requires std::ranges::input_range<Range> &&
                 std::same_as<std::ranges::range_value_t<Range>, key_type>
    auto erase(Range&& keys) -> int
    {
        int count = 0;
        for (const auto& key : keys) {
            // 如果本来就过期了，那么就不算删除成功
            if (erase_if_expired(key))
                continue;

            auto it = data_.find(key);
            if (it != data_.end()) {
                data_.erase(it);
                expire_time_.erase(key);
                ++count;
            }
        }
        return count;
    }

    auto expire_at(const key_type& key, time_t seconds) -> bool;
    // time to live
    auto ttl(const key_type& key) -> std::optional<time_t>;

    auto contains(const key_type& key) -> bool;

    void flush();

    void flush_async();

    void persist(const key_type& key);

    [[nodiscard]]
    constexpr auto size() const noexcept -> size_type
    {
        return data_.size();
    }

private:
    std::map<key_type, value_type> data_;
    std::unordered_map<key_type, time_t> expire_time_;

    static auto now() -> time_t;

    auto erase_if_expired(const key_type& key) -> bool;
};

} // namespace xin::redis

#endif // XIN_REDIS_STORAGE_H