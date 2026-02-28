#ifndef XIN_BASE_RANDOM_H
#define XIN_BASE_RANDOM_H

#include <algorithm>
#include <cassert>
#include <concepts>
#include <random>
#include <ranges>
#include <type_traits>
#include <vector>

namespace xin::base {

namespace detail {

auto engine() -> std::mt19937&;

} // namespace detail

struct random {
    random() = delete;

    static void seed(uint32_t value) { detail::engine().seed(value); }

    template<std::integral T = int>
    static auto uniform(T low, T high) -> T
    {
        std::uniform_int_distribution<T> dist{ low, high - 1 };
        return dist(detail::engine());
    }

    template<std::integral T = int>
    static auto uniform(T high) -> T
    {
        std::uniform_int_distribution<T> dist{ T{}, high - 1 };
        return dist(detail::engine());
    }

    template<std::floating_point T = double>
    static auto uniform(T low, T high) -> T
    {
        std::uniform_real_distribution<T> dist{ low, high - std::numeric_limits<T>::epsilon() };
        return dist(detail::engine());
    }

    template<std::floating_point T = double>
    static auto uniform(T high) -> T
    {
        std::uniform_real_distribution<T> dist{ T{}, high - std::numeric_limits<T>::epsilon() };
        return dist(detail::engine());
    }

    static auto bernoulli(double percentage = 0.5) -> bool
    {
        std::bernoulli_distribution dist(percentage);
        return dist(detail::engine());
    }

    template<std::integral T = int>
    static auto binomial(T n, double percentage) -> T
    {
        std::binomial_distribution<T> dist{ n, percentage };
        return dist(detail::engine());
    }

    template<std::floating_point T = double>
    static auto normal(T mean = 0.0, T stddev = 1.0) -> T
    {
        std::normal_distribution<T> dist{ mean, stddev };
        return dist(detail::engine());
    }

    template<std::floating_point T = double>
    static auto exponential(T lambda = 1.0) -> T
    {
        std::exponential_distribution<T> dist(lambda);
        return dist(detail::engine());
    }

    template<std::ranges::range Range>
    static void shuffle(Range&& range)
    {
        std::ranges::shuffle(range, detail::engine());
    }

    template<std::ranges::range Range>
    static auto choice(Range&& range) -> typename std::remove_reference_t<Range>::const_reference
    {
        assert(!std::ranges::empty(range));

        auto index = uniform(std::ranges::size(range));
        auto it = std::ranges::begin(range);
        std::advance(it, index);
        return *it;
    }

    template<std::ranges::range Range>
    static auto sample(Range&& range, typename std::remove_reference_t<Range>::size_type count)
    {
        assert(count <= std::ranges::size(range));

        using Result = std::vector<typename std::remove_reference_t<Range>::value_type>;

        Result vec{ std::ranges::begin(range), std::ranges::end(range) };
        shuffle(vec);

        return Result{ vec.begin(), vec.begin() + count };
    }
};

} // namespace xin::base

#endif // XIN_BASE_RANDOM_H