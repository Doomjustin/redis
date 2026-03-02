#ifndef XIN_REDIS_COMMAND_H
#define XIN_REDIS_COMMAND_H

#include <redis_command_define.h>

namespace xin::redis {
struct commands {
    commands() = delete;

    static auto dispatch(const Arguments& args) -> ResponsePtr;
};

} // namespace xin::redis

#endif // XIN_REDIS_COMMAND_H