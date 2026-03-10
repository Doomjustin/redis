#ifndef XIN_BASE_LRU_CACHE_H
#define XIN_BASE_LRU_CACHE_H

#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

namespace xin::base {

template<typename Key, typename Value>
class LRUCache {
private:
    struct Node;

public:
    using KeyType = Key;
    using ValueType = Value;
    using SizeType = std::size_t;

    LRUCache(SizeType capacity)
        : capacity_{ capacity }
    {
        head_->next = tail_.get();
        tail_->prev = head_.get();
    }

    LRUCache(const LRUCache&) = delete;
    auto operator=(const LRUCache&) -> LRUCache& = delete;

    LRUCache(LRUCache&&) noexcept = default;
    auto operator=(LRUCache&&) noexcept -> LRUCache& = default;

    ~LRUCache() = default;

    auto get(const KeyType& key) noexcept -> std::optional<ValueType>
    {
        if (!cache_.contains(key))
            return {};

        auto node = cache_[key].get();
        move_to_head(node);
        return node->value;
    }

    void put(KeyType key, ValueType value)
    {
        if (cache_.contains(key)) {
            auto node = cache_[key].get();
            node->value = std::move(value);
            move_to_head(node);
        }
        else {
            if (cache_.size() == capacity_) {
                auto tail = remove_tail();
                cache_.erase(tail->key);
            }

            auto new_node = std::make_unique<Node>(std::move(key), std::move(value));
            add_to_head(new_node.get());
            cache_[new_node->key] = std::move(new_node);
        }
    }

    [[nodiscard]]
    constexpr auto size() const noexcept -> SizeType
    {
        return cache_.size();
    }

    [[nodiscard]]
    constexpr auto capacity() const noexcept -> SizeType
    {
        return capacity_;
    }

private:
    SizeType capacity_ = 0;
    std::unique_ptr<Node> head_ = std::make_unique<Node>();
    std::unique_ptr<Node> tail_ = std::make_unique<Node>();
    std::unordered_map<KeyType, std::unique_ptr<Node>> cache_;

    void add_to_head(Node* node)
    {
        node->prev = head_.get();
        node->next = head_->next;
        head_->next->prev = node;
        head_->next = node;
    }

    void remove(Node* node)
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void move_to_head(Node* node)
    {
        remove(node);
        add_to_head(node);
    }

    auto remove_tail() -> Node*
    {
        Node* tail = tail_->prev;
        remove(tail);
        return tail;
    }
};

template<typename Key, typename Value>
struct LRUCache<Key, Value>::Node {
    Key key;
    Value value;
    Node* prev = nullptr;
    Node* next = nullptr;

    Node(Key key, Value value)
        : key{ std::move(key) }
        , value{ std::move(value) }
    {
    }

    Node() = default;
};

} // namespace xin::base

#endif // XIN_BASE_LRU_CACHE_H