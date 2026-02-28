#ifndef XIN_BASE_SPINLOCK_MUTEX_H
#define XIN_BASE_SPINLOCK_MUTEX_H

#include <atomic>

namespace xin::base {

class SpinlockMutex {
public:
    void lock()
    {
        while (flag.test_and_set(std::memory_order_acquire)) {
        }
    }

    void unlock() { flag.clear(std::memory_order_release); }

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

} // namespace xin::base

#endif // XIN_BASE_SPINLOCK_MUTEX_H