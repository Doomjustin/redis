#include "redis_storage.h"

namespace xin::redis {

void Database::set(key_type key, value_type value)
{
    if (expire_time_.contains(key))
        expire_time_.erase(key);

    data_.insert_or_assign(std::move(key), std::move(value));
}

auto Database::get(const key_type& key) -> std::optional<value_type>
{
    if (erase_if_expired(key))
        return {};

    auto it = data_.find(key);
    if (it != data_.end())
        return it->second;

    return {};
}

auto Database::expire_at(const key_type& key, time_t seconds) -> bool
{
    if (!data_.contains(key))
        return false;

    expire_time_.insert_or_assign(key, now() + seconds * 1000);
    return true;
}

auto Database::ttl(const key_type& key) -> std::optional<time_t>
{
    if (erase_if_expired(key))
        return {};

    auto it = expire_time_.find(key);
    if (it == expire_time_.end())
        return {};

    auto remaining = it->second > now() ? it->second - now() : 0;
    return remaining / 1000; // 转换为秒
}

auto Database::now() -> time_t
{
    auto now = clock::now();
    auto epoch = now.time_since_epoch();
    return std::chrono::duration_cast<time_unit>(epoch).count();
}

auto Database::erase_if_expired(const key_type& key) -> bool
{
    auto it = expire_time_.find(key);
    if (it != expire_time_.end() && now() >= it->second) {
        data_.erase(key);
        expire_time_.erase(key);
        return true;
    }

    return false;
}

} // namespace xin::redis