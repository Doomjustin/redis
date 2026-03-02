#ifndef XIN_REDIS_SORTED_SET_H
#define XIN_REDIS_SORTED_SET_H

#include <memory>
#include <set>
#include <unordered_map>

namespace xin::redis {

class SortedSet {
public:
    using Score = double;
    using Member = std::shared_ptr<std::string>;
    struct Data {
        Score score;
        Member member;

        auto operator<(const Data& other) const -> bool
        {
            if (score != other.score)
                return score < other.score;

            return *member < *other.member;
        }
    };

    using Iterator = std::set<Data>::iterator;

    auto insert_or_assign(Score score, Member member) -> bool;

    auto begin() const -> Iterator { return scores_tree_.begin(); }

    auto end() const -> Iterator { return scores_tree_.end(); }

    [[nodiscard]]
    constexpr auto size() const noexcept -> std::size_t
    {
        return scores_tree_.size();
    }

    [[nodiscard]]
    constexpr auto empty() const noexcept -> bool
    {
        return scores_tree_.empty();
    }

private:
    std::unordered_map<Member, Score> members_dict_;
    std::set<Data> scores_tree_;
};

} // namespace xin::redis

#endif // XIN_REDIS_SORTED_SET_H