#ifndef XIN_REDIS_COMMAND_HASH_TABLE_H
#define XIN_REDIS_COMMAND_HASH_TABLE_H

#include <redis_command_define.h>

namespace xin::redis {

struct hash_table_commands {
    hash_table_commands() = delete;

    // hset key field value [field value ...]
    // If key does not exist, a new key holding a hash is created.
    // If key holds a hash, the specified field is set to the specified value.
    // If key holds a value that is not a hash, an error is returned.
    // If field already exists in the hash, it is overwritten.
    // If field is a new field in the hash, it is added to the hash.
    // The command returns the number of fields that were added to the hash, not including fields
    // that were overwritten. If the command is called with an odd number of arguments, or if the
    // number of arguments is less than 4, an error is returned.
    static auto set(const Arguments& args) -> ResponsePtr;

    // hget key field
    // If the key does not exist, it returns a Null Bulk String.
    // If the key exists but does not hold a hash value, it returns a WRONGTYPE error.
    // If the key exists and holds a hash value, but the specified field does not exist in the hash,
    // it returns a Null Bulk String. If the key exists and holds a hash value, and the specified
    // field exists in the hash, it returns the value associated with the specified field.
    static auto get(const Arguments& args) -> ResponsePtr;

    // hgetall key
    // If the key does not exist, it returns an empty list.
    // If the key exists but does not hold a hash value, it returns a WRONGTYPE error.
    // If the key exists and holds a hash value, it returns a list of fields and their associated
    // values stored in the hash, in the form of a list where every field name is followed by its
    // value. Because of this, the number of elements in the returned list is twice the number of
    // fields in the hash.
    static auto get_all(const Arguments& args) -> ResponsePtr;
};

} // namespace xin::redis

#endif // XIN_REDIS_COMMAND_HASH_TABLE_H