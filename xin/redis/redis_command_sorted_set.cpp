#include "redis_command_sorted_set.h"

#include <base_formats.h>
#include <base_log.h>
#include <base_string_utility.h>
#include <redis_command_define.h>
#include <redis_response.h>

#include <memory>

using namespace xin::base;

namespace {

using namespace xin::redis;

using SortedSet = Database::SortedSetType;
using SortedSetPtr = Database::SortedSetPtr;

auto create_new_sorted_set(const Arguments& args) -> ResponsePtr
{
    auto sorted_set = std::make_shared<SortedSet>();

    int new_elements = 0;
    for (size_t i = 2; i + 1 < args.size(); i += 2) {
        auto score = numeric_cast<double>(args[i]);
        if (!score) {
            log::error("ZADD command received invalid score: {}", args[i]);
            return std::make_unique<ErrorResponse>("ERR value is not a valid float");
        }

        if (sorted_set->insert_or_assign(*score, std::make_shared<StringType>(args[i + 1])))
            ++new_elements;
    }

    db().set(args[1], std::move(sorted_set));

    log::info("ZADD command executed with key: {}, created new sorted set, added {} new elements",
              args[1], new_elements);

    return std::make_unique<IntegralResponse>(new_elements);
}

auto add_to_existing_sorted_set(const Arguments& args, SortedSet& sorted_set) -> ResponsePtr
{
    int new_elements = 0;
    for (size_t i = 2; i + 1 < args.size(); i += 2) {
        auto score = numeric_cast<double>(args[i]);
        if (!score) {
            log::error("ZADD command received invalid score: {}", args[i]);
            return std::make_unique<ErrorResponse>("ERR value is not a valid float");
        }

        if (sorted_set.insert_or_assign(*score, std::make_shared<StringType>(args[i + 1])))
            ++new_elements;
    }

    log::info("ZADD command executed with key: {}, updated existing sorted set, added {} new "
              "elements",
              args[1], new_elements);

    return std::make_unique<IntegralResponse>(new_elements);
}

auto range(const Arguments& args, bool with_scores) -> ResponsePtr
{
    auto res = db().get(args[1]);
    if (!res) {
        log::info("ZRANGE command executed with key: {}, but key does not exist", args[1]);
        return std::make_unique<BulkStringResponse>();
    }

    if (auto* sorted_set = std::get_if<SortedSetPtr>(&*res)) {
        auto& container = **sorted_set;

        auto start_opt = numeric_cast<std::int64_t>(args[2]);
        auto stop_opt = numeric_cast<std::int64_t>(args[3]);
        if (!start_opt || !stop_opt) {
            log::error("ZRANGE command received invalid start or stop index: {}, {}", args[2],
                       args[3]);
            return std::make_unique<ErrorResponse>(INVALID_INTEGRAL_ERR);
        }

        auto [start, stop] = normalize_range(*start_opt, *stop_opt, container.size());
        if (start > stop || start >= static_cast<std::int64_t>(container.size()))
            return std::make_unique<BulkStringResponse>();

        auto response = std::make_unique<BulkStringResponse>();
        for (auto it = std::next(container.begin(), start);
             it != std::next(container.begin(), stop + 1); ++it) {
            log::info("ZRANGE command executed with key: {}, returned member: {}", args[1],
                      *it->member);

            response->add_record(it->member);

            if (with_scores)
                response->add_record(std::make_shared<std::string>(xformat("{:17g}", it->score)));
        }

        log::info("ZRANGE command executed with key: {}, total {} members returned", args[1],
                  response->size());
        return response;
    }

    log::error(
        "ZRANGE command executed with key: {}, but WRONGTYPE Operation against a key holding "
        "the wrong kind of value",
        args[1]);
    return std::make_unique<ErrorResponse>(WRONG_TYPE_ERR);
}

} // namespace

namespace xin::redis {

auto sorted_set_commands::add(const Arguments& args) -> ResponsePtr
{
    if (args.size() < 4 || args.size() % 2 != 0) {
        log::error("ZADD command received wrong number of arguments: {}", args);
        return std::make_unique<ErrorResponse>(arguments_size_error("zadd"));
    }

    auto res = db().get(args[1]);
    if (!res) {
        log::info(
            "ZADD command executed with key: {}, but key does not exist, creating new sorted set",
            args[1]);
        return create_new_sorted_set(args);
    }

    if (auto* sorted_set = std::get_if<SortedSetPtr>(&*res))
        return add_to_existing_sorted_set(args, **sorted_set);

    log::error("ZADD command executed with key: {}, but WRONGTYPE Operation against a key holding "
               "the wrong kind of value",
               args[1]);
    return std::make_unique<ErrorResponse>(WRONG_TYPE_ERR);
}

auto sorted_set_commands::range(const Arguments& args) -> ResponsePtr
{
    if (args.size() == 5 && strings::to_lowercase(args[4]) != "withscores") {
        log::error("ZRANGE command received invalid option: {}", args[4]);
        return std::make_unique<ErrorResponse>("ERR syntax error");
    }

    if (args.size() == 4)
        return ::range(args, false);

    if (args.size() == 5 && strings::to_lowercase(args[4]) == "withscores")
        return ::range(args, true);

    return std::make_unique<ErrorResponse>(arguments_size_error("zrange"));
}

} // namespace xin::redis