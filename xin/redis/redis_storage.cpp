#include "redis_storage.h"

#include <thread>

namespace xin::redis {

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

auto Database::contains(const key_type& key) -> bool
{
    if (erase_if_expired(key))
        return false;

    return data_.contains(key);
}

void Database::flush()
{
    data_.clear();
    expire_time_.clear();
}

void Database::flush_async()
{
    std::map<key_type, value_type> old_data;
    std::unordered_map<key_type, time_t> old_expire_time;
    std::swap(old_data, data_);
    std::swap(old_expire_time, expire_time_);

    // 在后台线程中清理旧数据
    auto cleanup_task = [](std::map<key_type, value_type> old_data,
                           std::unordered_map<key_type, time_t> old_expire_time) {
        old_data.clear();
        old_expire_time.clear();
    };

    std::thread t{ cleanup_task, std::move(old_data), std::move(old_expire_time) };
    t.detach();
}

auto Database::persist(const key_type& key) -> bool { return expire_time_.erase(key) > 0; }

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