#include <iostream>
#include <thread>
#include <future>
#include <random>
#include <condition_variable>
#include <mutex>
#include <cassert>
#include <atomic>
#include <string>

#include <semaphore.h>
#include <pthread.h>

#define SINGLE_CORE 0
#define ATOMIC_TEST 0
#define CPU_FENCE 0

using namespace std;

void test_reorder() {
    cout << "Test reorder ..." << endl;
    int x;
    int y;
    atomic_int ax{0};
    atomic_int ay{0};
    int regx = 0;
    int regy = 0;
    sem_t semx;
    sem_t semy;
    sem_t end;
    mutex lx;
    mutex ly;
    bool stop{false};

    auto worker_set_x = [&]() {
        uniform_int_distribution<int> dist(0, 100);
        random_device rd;
        while (!stop) {
            sem_wait(&semx);
            while (dist(rd) % 8 != 0) {

            }

            //lock_guard locker{lx};
#if ATOMIC_TEST
            ax.store(1, memory_order_relaxed);
#else
            x = 1;
#endif
            asm volatile("" ::: "memory");
#if ATOMIC_TEST
            regy = ay.load(memory_order_relaxed);
#else
            regy = y;
#endif
            sem_post(&end);
        }
    };
    auto worker_set_y = [&]() {
        uniform_int_distribution<int> dist(0, 100);
        random_device rd;
        while (!stop) {
            sem_wait(&semy);
            while (dist(rd) % 8 != 0) {

            }

            //lock_guard locker{lx}; // must be same mutex, protect same critical section
#if ATOMIC_TEST
            ay.store(1, memory_order_relaxed);
#else
            y = 1;
#endif
            asm volatile("" ::: "memory");
#if ATOMIC_TEST
            regx = ax.load(memory_order_relaxed);
#else
            regx = x;
#endif
            sem_post(&end);
        }
    };

    sem_init(&semx, 0, 0);
    sem_init(&semy, 0, 0);
    sem_init(&end, 0, 0);

    thread tx(worker_set_x);
    thread ty(worker_set_y);
    auto reorder = 0;

//#if defined(SINGLE_CORE)
#if SINGLE_CORE
    // 2. single cpu core
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(0, &cpus);
    pthread_setaffinity_np(tx.native_handle(), sizeof(cpu_set_t), &cpus);
    pthread_setaffinity_np(ty.native_handle(), sizeof(cpu_set_t), &cpus);
#endif

    for (auto i = 1; i < 100000; ++i) {
        x = 0;
        y = 0;
        sem_post(&semx);
        sem_post(&semy);

        sem_wait(&end);
        sem_wait(&end);
        if (regx == 0 && regy == 0) {
            ++reorder;
            cout << "cpu has reorder " << reorder << " after " << i << " iterations" << endl;
        }
    }

    stop = true;
    sem_post(&semx);
    sem_post(&semy);

    tx.join();
    ty.join();
}

void test_seq_cst() {
    cout << "Test memory_order_seq_cst ..." << endl;
    for (auto i = 0; ; ++i) {

    atomic_bool x{false};
    atomic_bool y{false};
    atomic_int z{0};

    thread a{[&]() {
        uniform_int_distribution<int> dist(0, 100);
        random_device rd;
        while (dist(rd) % 8 != 0)
            ;
        x.store(true, memory_order_relaxed);
    }};
    thread b{[&]() {
        uniform_int_distribution<int> dist(0, 100);
        random_device rd;
        while (dist(rd) % 8 != 0)
            ;
        y.store(true, memory_order_relaxed);
    }};
    thread c{[&]() {
        while (!x.load(memory_order_relaxed))
            ;
        if (y.load(memory_order_relaxed)) {
            ++z;
        }
    }};
    thread d{[&]() {
        while (!y.load(memory_order_relaxed))
            ;
        if (x.load(memory_order_relaxed)) {
            ++z;
        }
    }};

    a.join();
    b.join();
    c.join();
    d.join();
    assert(z != 0);
    }
}

void test_acq_rel() {
    cout << "Test acq_rel ..." << endl;
    for (auto i = 0;;++i) {

    atomic<string*> ptr;
    int data;

    auto producer = [&]() {
        std::string* p  = new std::string("Hello");
        data = 42;
        ptr.store(p, std::memory_order_relaxed);
    };

    auto consumer = [&]() {
        std::string* p2;
        while (!(p2 = ptr.load(std::memory_order_relaxed)))
            ;
        assert(*p2 == "Hello");
        assert(data == 42);
    };

    thread t1{producer};
    thread t2{consumer};

    t1.join();
    t2.join();
    }
}
