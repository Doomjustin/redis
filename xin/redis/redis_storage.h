#ifndef XIN_REDIS_STORAGE_H
#define XIN_REDIS_STORAGE_H

#include <chrono>
#include <concepts>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace xin::redis {

class Database {
public:
    using key_type = std::string;
    using string_type = std::shared_ptr<std::string>;
    using hash_type = std::shared_ptr<std::unordered_map<key_type, string_type>>;
    using list_type = std::shared_ptr<std::list<string_type>>;
    using value_type = std::variant<string_type, hash_type, list_type>;
    using clock = std::chrono::steady_clock;
    using time_point = clock::time_point;
    using duration = clock::duration;
    using time_unit = std::chrono::milliseconds;
    using time_t = std::uint64_t;
    using size_type = std::size_t;

    template<typename T>
    static constexpr auto is_valid_value =
        std::is_same_v<T, string_type> || std::is_same_v<T, hash_type> ||
        std::is_same_v<T, list_type>;

    template<typename Value>
        requires is_valid_value<Value>
    void set(key_type key, Value value)
    {
        if (expire_time_.contains(key))
            expire_time_.erase(key);

        data_.insert_or_assign(std::move(key), std::move(value));
    }

    template<typename Value>
        requires is_valid_value<Value>
    void set(key_type key, Value value, time_t expire_seconds)
    {
        expire_time_.insert_or_assign(key, now() + expire_seconds * 1000);
        data_.insert_or_assign(std::move(key), std::move(value));
    }

    template<typename Value>
        requires is_valid_value<Value>
    auto get_if(const key_type& key) -> std::optional<Value>
    {
        if (erase_if_expired(key))
            return {};

        auto it = data_.find(key);
        if (it != data_.end() && std::holds_alternative<Value>(it->second))
            return std::get<Value>(it->second);

        return {};
    }

    auto get(const key_type& key) -> std::optional<value_type>;

    template<typename Range>
        requires std::ranges::input_range<Range> &&
                 std::same_as<std::ranges::range_value_t<Range>, key_type>
    auto keys(Range&& keys) -> std::vector<key_type>
    {
        std::vector<key_type> result{};
        for (const auto& key : keys) {
            if (contains(key))
                result.push_back(key);
        }
        return result;
    }

    template<typename Range>
        requires std::ranges::input_range<Range> &&
                 std::same_as<std::ranges::range_value_t<Range>, key_type>
    auto mget(Range&& keys) -> std::vector<string_type>
    {
        std::vector<string_type> result{};
        for (const auto& key : keys) {
            if (auto res = get_if<string_type>(key))
                result.push_back(*res);
            else
                result.push_back(nullptr);
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

    auto persist(const key_type& key) -> bool;

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