#include <mem/lock.h>
#include <mem/lock_set.h>
#include <mem/lock_pool.h>
#include <utils/test.h>

#include <iostream>
#include <vector>
#include <thread>

using namespace std;


class SyncInt : public LockObject {
    public:
    volatile int v = 0;
};

class SyncIntSet : public SetLock<int> {
    public:
    volatile int v = 0;
};

class SyncIntPool : public PoolLock<int, 16> {
    public:
    volatile int v = 0;
};

template<class Tl>
void many_inc_thread(Tl *v, int nit) {
    for (int i=0; i<nit; i++) {
        auto l = v->lock();
        v->v++;
    }
}

void test_many_inc(int nth, int nit) {
    SyncInt v;
    vector<thread> T;
    for (int i = 0; i < nth; i++) {
        T.emplace_back(many_inc_thread<decltype(v)>, &v, nit);
    }
    for (auto &t : T) {
        t.join();
    }
    ASSERT(v.v == nth * nit, "Invalid count", v.v);
}

template<class Tl, class Ta>
void many_inc_thread_arg(Tl *v, Ta arg, int nit) {
    for (int i=0; i<nit; i++) {
        auto l = v->lock(arg);
        v->v++;
    }
}

template<class Tl>
void test_many_inc_arg(int nth, int nit) {
    Tl v;
    vector<thread> T;
    for (int i = 0; i < nth; i++) {
        T.emplace_back(many_inc_thread_arg<decltype(v), int>, &v, 0, nit);
    }
    for (auto &t : T) {
        t.join();
    }
    ASSERT(v.v == nth * nit, "Invalid count", v.v);
}

int test() {
    INFO(test) << "Object size: " << sizeof(LockObject) << DBG_ENDL;
    TEST(test_many_inc).benchmark(50, 10, 1e6);
    TEST(test_many_inc_arg<SyncIntSet>).benchmark(10, 10, 1e5);
    TEST(test_many_inc_arg<SyncIntPool>).benchmark(50, 10, 1e6);
    return 0;
}
