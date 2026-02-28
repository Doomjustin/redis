#include "base_random.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <string>
#include <vector>

TEST_SUITE("base-random")
{
    using namespace xin::base;

    TEST_CASE("shuffle")
    {
        std::vector<int> vec{ 1, 2, 3, 4, 5 };
        auto original_vec = vec;

        random::seed(42); // 固定随机种子以确保测试可重复
        random::shuffle(vec);

        REQUIRE(vec != original_vec); // 确保顺序被打乱
        REQUIRE(std::is_permutation(vec.begin(), vec.end(),
                                    original_vec.begin())); // 确保元素未丢失或重复
    }

    TEST_CASE("choice")
    {
        std::vector<std::string> vec{ "apple", "banana", "cherry", "date", "elderberry" };

        random::seed(42); // 固定随机种子以确保测试可重复
        auto choice1 = random::choice(vec);
        auto choice2 = random::choice(vec);

        REQUIRE(choice1 == "banana");
        REQUIRE(choice2 == "date");
    }

    TEST_CASE("sample")
    {
        std::vector<int> vec{ 1, 2, 3, 4, 5 };

        random::seed(42); // 固定随机种子以确保测试可重复
        auto sample = random::sample(vec, 3);

        REQUIRE(sample.size() == 3);
        REQUIRE(std::ranges::all_of(sample, [&](int x) {
            return std::ranges::find(vec, x) != vec.end();
        })); // 确保采样的元素来自原始范围
    }
}