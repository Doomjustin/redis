#ifndef XIN_REDIS_COMMAND_H
#define XIN_REDIS_COMMAND_H

#include "redis_response.h"
#include "redis_serialization_protocl.h"

namespace xin::redis {

struct commands {
    commands() = delete;

    using arguments = RESPParser::arguments;
    using response = std::unique_ptr<Response>;

    static auto dispatch(const arguments& args) -> response;
};

} // namespace xin::redis

#endif // XIN_REDIS_COMMAND_H