#ifndef XIN_REDIS_COMMAND_LIST_H
#define XIN_REDIS_COMMAND_LIST_H

#include "redis_command_define.h"

namespace xin::redis {

struct list_commands {
    list_commands() = delete;

    // lpush key value [value ...]
    // If key does not exist, a new key holding a list is created.
    // If key holds a list, the specified values are inserted at the head of the list.
    // If key holds a value that is not a list, an error is returned.
    // The command returns the length of the list after the push operations.
    // If the command is called with less than 3 arguments, an error is returned.
    static auto push(const arguments& args) -> response;

    // lpop key
    // If the key does not exist, it returns a Null Bulk String.
    // If the key exists but does not hold a list value, it returns a WRONGTYPE error.
    // If the key exists and holds a list value, but the list is empty, it returns a Null Bulk
    // String. If the key exists and holds a list value, and the list is not empty, it removes and
    // returns the first element of the list.
    static auto pop(const arguments& args) -> response;

    // lrange key start stop
    // If the key does not exist, it returns an empty list.
    // If the key exists but does not hold a list value, it returns a WRONGTYPE error.
    // If the key exists and holds a list value, it returns the specified elements of the list
    // stored at the key. The offsets start and stop are zero-based indexes, with 0 being the first
    // element of the list (the head of the list), 1 being the next element and so on. These offsets
    // can also be negative numbers indicating offsets starting at the end of the list.
    static auto range(const arguments& args) -> response;
};

} // namespace xin::redis

#endif // XIN_REDIS_COMMAND_LIST_H