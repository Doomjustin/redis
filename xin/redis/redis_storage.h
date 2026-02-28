#ifndef XIN_REDIS_STORAGE_H
#define XIN_REDIS_STORAGE_H

#include <map>
#include <memory>
#include <string>

namespace xin::redis {

static std::map<std::string, std::shared_ptr<std::string>> storage;

} // namespace xin::redis

#endif // XIN_REDIS_STORAGE_H