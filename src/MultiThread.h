#pragma once

#include <atomic>
#include <cassert>

class SpinLock {
public:
    SpinLock() = default;
    void Lock() {
        //int i = 0;
        //while (!d_.compare_exchange_strong(i, 1)) {
        //    i = 0;
        //}
        while (!f_.test_and_set())
            ;
    }
    void Unlock() {
        //assert(d_.load(std::memory_order_relaxed) == 1);
        //d_.store(0, std::memory_order_relaxed);
        f_.clear();
    }

private:
    std::atomic_int d_{0};
    std::atomic_flag f_;
};

