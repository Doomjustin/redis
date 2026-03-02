#ifndef XIN_REDIS_COMMAND_SORTED_SET_H
#define XIN_REDIS_COMMAND_SORTED_SET_H

#include "redis_command_define.h"

namespace xin::redis {

struct sorted_set_commands {
    sorted_set_commands() = delete;

    // ZADD key score1 member1 [score2 member2 ...]
    // 返回新添加的元素数量（不包括更新分数但成员已存在的元素）
    // 如果key不存在，那么就创建一个新的sorted set并插入成员分数对
    // 如果key存在且是sorted set类型，那么就更新sorted set中的成员分数对
    // 如果key存在但不是sorted set类型，那么就返回错误
    // score必须是可以转换为double的字符串，否则返回错误
    // 成员是一个字符串，分数是一个double，sorted
    // set中的元素按照分数从小到大排序，如果分数相同则按照成员的字典序排序
    // 例如：ZADD myzset 1.0 one 2.0 two 3.0 three
    // 返回myzset中新增的元素数量，如果myzset之前不存在，那么返回3
    static auto add(const Arguments& args) -> ResponsePtr;

    // ZRANGE key start stop [WITHSCORES]
    // 返回指定范围内的元素，按分数从小到大排序
    // start和stop是索引，支持负数索引，-1表示最后一个元素
    // 如果指定了WITHSCORES选项，那么返回的元素会包含分数，格式为：member1 score1 member2 score2 ...
    // 例如：ZRANGE myzset 0 -1 WITHSCORES  返回myzset中所有元素和分数
    static auto range(const Arguments& args) -> ResponsePtr;
};

} // namespace xin::redis

#endif // XIN_REDIS_COMMAND_SORTED_SET_H