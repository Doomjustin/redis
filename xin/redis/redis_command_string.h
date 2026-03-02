#ifndef XIN_REDIS_COMMAND_STRING_H
#define XIN_REDIS_COMMAND_STRING_H

#include "redis_command_define.h"

namespace xin::redis {

struct string_commands {
    string_commands() = delete;

    // set key value [EX seconds]
    // If the command is called with 3 arguments, it sets the key to the specified value without an
    // expiration time. If the command is called with 5 arguments and the fourth argument is "EX",
    // it sets the key to the specified value and sets the expiration time to the number of seconds
    // specified in the fifth argument.
    static auto set(const arguments& args) -> response;

    // get key
    // If the key exists and holds a string value, it returns the value of the key.
    // If the key does not exist, it returns a Null Bulk String.
    // If the key exists but does not hold a string value, it returns a WRONGTYPE error.
    static auto get(const arguments& args) -> response;
};

} // namespace xin::redis

#endif // XIN_REDIS_COMMAND_STRING_H