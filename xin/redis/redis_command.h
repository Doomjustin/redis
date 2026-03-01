#ifndef XIN_REDIS_COMMAND_H
#define XIN_REDIS_COMMAND_H

#include "redis_response.h"
#include "redis_serialization_protocl.h"

#include <functional>

namespace xin::redis {

struct commands {
    commands() = delete;

    using arguments = RESPParser::arguments;
    using response = std::unique_ptr<Response>;
    using handler = std::function<response(const arguments&)>;

    static auto dispatch(const arguments& args) -> response;

    static auto set(const arguments& args) -> response;

    static auto get(const arguments& args) -> response;

    static auto ping(const arguments& args) -> response;
};

} // namespace xin::redis

#endif // XIN_REDIS_COMMAND_H