#include "redis_sorted_set.h"

namespace xin::redis {

auto SortedSet::insert_or_assign(Score score, Member member) -> bool
{
    auto it = members_dict_.find(member);
    if (it != members_dict_.end()) {
        // 如果成员已经存在且分数相同，则不需要更新
        if (it->second == score)
            return false;

        scores_tree_.erase(Data{ .score = it->second, .member = member });
    }

    members_dict_[member] = score;
    scores_tree_.insert(Data{ .score = score, .member = member });
    return true;
}

} // namespace xin::redis