#ifndef XIN_BASE_LFU_CACHE_H
#define XIN_BASE_LFU_CACHE_H

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>

namespace xin::base {

template<typename Key, typename Value>
class LFUCache {
public:
    using Size = std::size_t;

    explicit LFUCache(Size capacity)
        : capacity_{ capacity }
    {
    }

    LFUCache(const LFUCache&) = delete;
    auto operator=(const LFUCache&) -> LFUCache& = delete;

    LFUCache(LFUCache&&) noexcept = default;
    auto operator=(LFUCache&&) noexcept -> LFUCache& = default;

    ~LFUCache() = default;

    auto get(const Key& key) -> std::optional<Value>
    {
        auto it = keys_cache_.find(key);
        if (it == keys_cache_.end())
            return {};

        auto iter = it->second;
        auto value = iter->value;
        touch(iter);
        return value;
    }

    void put(Key key, Value value)
    {
        if (capacity_ == 0)
            return;

        auto found = keys_cache_.find(key);
        if (found != keys_cache_.end()) {
            auto iter = found->second;
            iter->value = std::move(value);
            touch(iter);
            return;
        }

        // 将频率最低的值key删除
        // 因为相同频率时，最近访问过的会放在front，所以从back删除即可
        if (keys_cache_.size() == capacity_) {
            auto& min_frequency_list = frequencies_[min_frequency_];
            keys_cache_.erase(min_frequency_list.back().key);
            min_frequency_list.pop_back();

            if (min_frequency_list.empty())
                frequencies_.erase(min_frequency_);
        }

        // 能走到这里说明一定是一个新key，频率为1
        min_frequency_ = 1;
        auto& list = frequencies_[1];
        list.emplace_front(key, std::move(value), 1);
        keys_cache_[key] = list.begin();
    }

    auto contains(const Key& key) const -> bool { return keys_cache_.contains(key); }

    [[nodiscard]]
    constexpr auto size() const noexcept -> Size
    {
        return keys_cache_.size();
    }

    [[nodiscard]]
    constexpr auto capacity() const noexcept -> Size
    {
        return capacity_;
    }

private:
    struct Node {
        Key key;
        Value value;
        Size frequency;
    };

    using List = std::list<Node>;
    using Iterator = typename List::iterator;

    Size capacity_ = 0;
    Size min_frequency_ = 0;
    std::unordered_map<Size, List> frequencies_;
    std::unordered_map<Key, Iterator> keys_cache_;

    void touch(Iterator iter)
    {
        const auto old_frequency = iter->frequency;
        auto& old_list = frequencies_[old_frequency];
        Node node{ *iter };
        old_list.erase(iter);

        if (old_list.empty()) {
            frequencies_.erase(old_frequency);
            if (min_frequency_ == old_frequency)
                ++min_frequency_;
        }

        node.frequency = old_frequency + 1;
        auto& new_list = frequencies_[node.frequency];
        new_list.push_front(node);
        keys_cache_[node.key] = new_list.begin();
    }
};

} // namespace xin::base

#endif // XIN_BASE_LFU_CACHE_H
