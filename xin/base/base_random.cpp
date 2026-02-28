#include "base_random.h"

namespace xin::base::detail {

auto engine() -> std::mt19937&
{
    static std::mt19937 engine{ std::random_device{}() };
    return engine;
}

} // namespace xin::base::detail