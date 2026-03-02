#include "redis_command_list.h"

#include "redis_command_define.h"

#include <base_log.h>

using namespace xin::base;

namespace {

using namespace xin::redis;

auto create_new_list(const Arguments& args) -> ResponsePtr
{
    log::info("LPUSH command executed with key: {}, but key does not exist, creating new list",
              args[1]);

    auto list = std::make_shared<ListPtr::element_type>();
    for (size_t i = 2; i < args.size(); ++i)
        list->push_front(std::make_shared<std::string>(args[i]));

    db().set(args[1], std::move(list));

    return std::make_unique<IntegralResponse>(args.size() - 2);
}

auto add_to_existing_list(const Arguments& args, ListType& container) -> ResponsePtr
{
    for (size_t i = 2; i < args.size(); ++i)
        container.push_front(std::make_shared<std::string>(args[i]));

    log::info("LPUSH command executed with key: {}, added {} elements to existing list", args[1],
              args.size() - 2);
    return std::make_unique<IntegralResponse>(container.size());
}

} // namespace

namespace xin::redis {

auto list_commands::push(const Arguments& args) -> ResponsePtr
{
    if (args.size() < 3) {
        log::error("LPUSH command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("lpush"));
    }

    auto res = db().get(args[1]);
    if (!res)
        return create_new_list(args);

    if (auto* list = std::get_if<ListPtr>(&*res))
        return add_to_existing_list(args, **list);

    log::error("LPUSH command executed with key: {}, but WRONGTYPE Operation against a "
               "key holding the wrong kind of value",
               args[1]);
    return std::make_unique<ErrorResponse>(WRONG_TYPE_ERR);
}

auto list_commands::pop(const Arguments& args) -> ResponsePtr
{
    if (args.size() != 2) {
        log::error("LPOP command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("lpop"));
    }

    auto res = db().get(args[1]);
    if (!res) {
        log::info("LPOP command executed with key: {}, but key does not exist", args[1]);
        return std::make_unique<NullBulkStringResponse>();
    }

    if (auto* list = std::get_if<ListPtr>(&*res)) {
        auto& container = **list;

        if (container.empty()) {
            log::info("LPOP command executed with key: {}, but list is empty", args[1]);
            return std::make_unique<NullBulkStringResponse>();
        }

        auto value = container.front();
        container.pop_front();

        log::info("LPOP command executed with key: {}, popped value: {}", args[1], *value);
        return std::make_unique<SingleBulkStringResponse>(value);
    }

    log::error("LPOP command executed with key: {}, but WRONGTYPE Operation against a "
               "key holding the wrong kind of value",
               args[1]);
    return std::make_unique<ErrorResponse>(WRONG_TYPE_ERR);
}

auto list_commands::range(const Arguments& args) -> ResponsePtr
{
    if (args.size() != 4) {
        log::error("LRANGE command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("lrange"));
    }

    auto res = db().get(args[1]);
    if (!res) {
        log::info("LRANGE command executed with key: {}, but key does not exist", args[1]);
        return std::make_unique<BulkStringResponse>();
    }

    if (auto* list = std::get_if<ListPtr>(&*res)) {
        auto& container = **list;

        auto start_opt = numeric_cast<std::int64_t>(args[2]);
        auto stop_opt = numeric_cast<std::int64_t>(args[3]);
        if (!start_opt || !stop_opt) {
            log::error("LRANGE command executed with key: {}, but invalid start or stop index: {} "
                       "or {}",
                       args[1], args[2], args[3]);
            return std::make_unique<ErrorResponse>(INVALID_INTEGRAL_ERR);
        }

        auto [start, stop] = normalize_range(*start_opt, *stop_opt, container.size());
        if (start > stop || start >= static_cast<std::int64_t>(container.size())) {
            log::error("LRANGE command executed with key: {}, but start index {} is greater than "
                       "stop index {} or out of range",
                       args[1], start, stop);
            return std::make_unique<BulkStringResponse>();
        }

        auto response = std::make_unique<BulkStringResponse>();
        for (auto beg = std::next(container.begin(), start),
                  end = std::next(container.begin(), stop + 1);
             beg != end; ++beg) {
            log::info("LRANGE command executed with key: {}, index: {}, value: {}", args[1],
                      std::distance(container.begin(), beg), **beg);
            response->add_record(*beg);
        }

        log::info("LRANGE command executed with key: {}, returned {} elements", args[1],
                  response->size());
        return response;
    }

    log::error("LRANGE command executed with key: {}, but WRONGTYPE Operation against a "
               "key holding the wrong kind of value",
               args[1]);
    return std::make_unique<ErrorResponse>(WRONG_TYPE_ERR);
}

} // namespace xin::redis