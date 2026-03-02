#ifndef XIN_REDIS_STORAGE_H
#define XIN_REDIS_STORAGE_H

#include <redis_sorted_set.h>

#include <chrono>
#include <concepts>
#include <list>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace xin::redis {

class Database {
public:
    using KeyType = std::string;
    using StringType = std::string;
    using StringPtr = std::shared_ptr<StringType>;
    using HashType = std::unordered_map<KeyType, StringPtr>;
    using HashPtr = std::shared_ptr<HashType>;
    using ListType = std::list<StringPtr>;
    using ListPtr = std::shared_ptr<ListType>;
    using SortedSetType = SortedSet;
    using SortedSetPtr = std::shared_ptr<SortedSetType>;
    using ValueType = std::variant<StringPtr, HashPtr, ListPtr, SortedSetPtr>;
    using SizeType = std::size_t;
    using Seconds = std::uint64_t;

    template<typename T>
    static constexpr auto is_valid_value =
        std::is_same_v<T, StringPtr> || std::is_same_v<T, HashPtr> || std::is_same_v<T, ListPtr> ||
        std::is_same_v<T, SortedSetPtr>;

    template<typename Value>
        requires is_valid_value<Value>
    void set(const KeyType& key, Value value)
    {
        if (expire_time_.contains(key))
            expire_time_.erase(key);

        data_.insert_or_assign(key, std::move(value));
    }

    template<typename Value>
        requires is_valid_value<Value>
    void set(const KeyType& key, Value value, Seconds expire_seconds)
    {
        expire_time_.insert_or_assign(key, now() + expire_seconds * 1000);
        data_.insert_or_assign(key, std::move(value));
    }

    template<typename Value>
        requires is_valid_value<Value>
    auto get_if(const KeyType& key) -> std::optional<Value>
    {
        if (erase_if_expired(key))
            return {};

        auto it = data_.find(key);
        if (it != data_.end() && std::holds_alternative<Value>(it->second))
            return std::get<Value>(it->second);

        return {};
    }

    auto get(const KeyType& key) -> std::optional<ValueType>;

    template<typename Range>
        requires std::ranges::input_range<Range> &&
                 std::same_as<std::ranges::range_value_t<Range>, KeyType>
    auto keys(Range&& keys) -> std::vector<KeyType>
    {
        std::vector<KeyType> result{};
        for (const auto& key : keys) {
            if (contains(key))
                result.push_back(key);
        }
        return result;
    }

    template<typename Range>
        requires std::ranges::input_range<Range> &&
                 std::same_as<std::ranges::range_value_t<Range>, KeyType>
    auto mget(Range&& keys) -> std::vector<StringPtr>
    {
        std::vector<StringPtr> result{};
        for (const auto& key : keys) {
            if (auto res = get_if<StringPtr>(key))
                result.push_back(*res);
            else
                result.push_back(nullptr);
        }
        return result;
    }

    template<typename Range>
        requires std::ranges::input_range<Range> &&
                 std::same_as<std::ranges::range_value_t<Range>, KeyType>
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

    auto expire_at(const KeyType& key, Seconds seconds) -> bool;
    // time to live
    auto ttl(const KeyType& key) -> std::optional<Seconds>;

    auto contains(const KeyType& key) -> bool;

    void flush();

    void flush_async();

    auto persist(const KeyType& key) -> bool;

    [[nodiscard]]
    constexpr auto size() const noexcept -> SizeType
    {
        return data_.size();
    }

private:
    using Clock = std::chrono::steady_clock;
    using TimeUnit = std::chrono::milliseconds;
    using Datas = std::unordered_map<KeyType, ValueType>;
    using Expires = std::unordered_map<KeyType, Seconds>;

    Datas data_;
    Expires expire_time_;

    static auto now() -> Seconds;

    auto erase_if_expired(const KeyType& key) -> bool;
};

} // namespace xin::redis

#endif // XIN_REDIS_STORAGE_H