#include "redis_storage.h"

#include <thread>

namespace xin::redis {

auto Database::get(const KeyType& key) -> std::optional<ValueType>
{
    if (erase_if_expired(key))
        return {};

    auto it = data_.find(key);
    if (it != data_.end())
        return it->second;

    return {};
}

auto Database::expire_at(const KeyType& key, Seconds seconds) -> bool
{
    auto data_it = data_.find(key);
    if (data_it == data_.end())
        return false;

    erase_expire(data_it->first);
    expires_.insert(std::make_pair(now() + seconds * 1000, std::string_view{ data_it->first }));
    return true;
}

auto Database::ttl(const KeyType& key) -> std::optional<Seconds>
{
    if (erase_if_expired(key))
        return {};

    auto it = find_expire_time(key);
    if (it == expires_.end())
        return {};

    auto remaining = it->first > now() ? it->first - now() : 0;
    return remaining / 1000; // 转换为秒
}

auto Database::contains(const KeyType& key) -> bool
{
    if (erase_if_expired(key))
        return false;

    return data_.contains(key);
}

void Database::flush()
{
    data_.clear();
    expires_.clear();
}

void Database::flush_async()
{
    Datas old_data;
    Expires old_expire_time;
    std::swap(old_data, data_);
    std::swap(old_expire_time, expires_);

    // 在后台线程中清理旧数据
    auto cleanup_task = [](Datas old_data, Expires old_expire_time) {
        old_data.clear();
        old_expire_time.clear();
    };

    std::thread t{ cleanup_task, std::move(old_data), std::move(old_expire_time) };
    t.detach();
}

auto Database::persist(const KeyType& key) -> bool
{
    auto it = find_expire_time(key);
    if (it != expires_.end()) {
        expires_.erase(it);
        return true;
    }

    return false;
}

auto Database::expired_keys() -> std::vector<std::string>
{
    auto expiry = now();
    std::vector<std::string> expired_keys;
    for (auto it = expires_.begin(); it != expires_.end();) {
        if (it->first > expiry)
            break;

        expired_keys.emplace_back(it->second);
        data_.erase(std::string{ it->second });
        it = expires_.erase(it);
    }

    return expired_keys;
}

auto Database::now() -> Seconds
{
    auto now = Clock::now();
    auto epoch = now.time_since_epoch();
    return std::chrono::duration_cast<TimeUnit>(epoch).count();
}

auto Database::erase_if_expired(const KeyType& key) -> bool
{
    auto it = find_expire_time(key);
    if (it != expires_.end() && now() >= it->first) {
        data_.erase(key);
        expires_.erase(it);
        return true;
    }

    return false;
}

auto Database::find_expire_time(std::string_view key) -> Expires::iterator
{
    for (auto it = expires_.begin(); it != expires_.end(); ++it) {
        if (it->second == key)
            return it;
    }

    return expires_.end();
}

void Database::erase_expire(std::string_view key)
{
    auto it = find_expire_time(key);
    if (it != expires_.end())
        expires_.erase(it);
}

} // namespace xin::redis