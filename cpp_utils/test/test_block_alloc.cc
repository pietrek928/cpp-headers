#include "mem/block_alloc.h"
#include "mem/secure_alloc.h"

#include "utils/test.h"

#include <thread>

using namespace std;

class TestObj {
    public:
        static BlockAlloc<TestObj> allocator;

        int v = 0;
        int inc() {
            return ++v;
        }
        int dec() {
            return --v;
        }
};
BlockAlloc<TestObj> TestObj::allocator;

class TestObjSecure {
    public:
        static BlockAlloc<TestObjSecure, SecureAllocator<>> allocator;

        int v = 0;
        int inc() {
            return ++v;
        }
        int dec() {
            return --v;
        }
};
BlockAlloc<TestObjSecure, SecureAllocator<>> TestObjSecure::allocator;

void test_alloc() {
    auto ref = TestObj::allocator.emplace();
    auto u = ref.use();
    u.obj().inc();
}

void test_alloc_secure() {
    auto ref = TestObjSecure::allocator.emplace();
    auto u = ref.use();
    u.obj().inc();
}

void many_alloc_thread(int nit) {
    for (int i=0; i<nit; i++) {
        auto ref = TestObj::allocator.emplace();
        auto u = ref.use();
        u.obj().inc();
    }
}

void test_many_alloc(int nth, int nit) {
    vector<thread> T;
    for (int i = 0; i < nth; i++) {
        T.emplace_back(many_alloc_thread, nit);
    }
    for (auto &t : T) {
        t.join();
    }
}

template<class Tr>
void many_inc_thread(Tr *v, int nit) {
    for (int i=0; i<nit; i++) {
        auto u = v->use();
        u.obj().inc();
    }
}

void test_many_inc(int nth, int nit) {
    auto ref = TestObj::allocator.emplace();
    vector<thread> T;
    for (int i = 0; i < nth; i++) {
        T.emplace_back(many_inc_thread<decltype(ref)>, &ref, nit);
    }
    for (auto &t : T) {
        t.join();
    }
    auto u = ref.use();
    ASSERT(u.obj().v == nth * nit, "Invalid count", u.obj().v);
}

//template<class Tl, class Ta>
//void many_inc_thread_arg(Tl *v, Ta arg, int nit) {
    //for (int i=0; i<nit; i++) {
        //auto l = v->lock(arg);
        //v->v++;
    //}
//}

//template<class Tl>
//void test_many_inc_arg(int nth, int nit) {
    //Tl v;
    //vector<thread> T;
    //for (int i = 0; i < nth; i++) {
        //T.emplace_back(many_inc_thread_arg<decltype(v), int>, &v, 0, nit);
    //}
    //for (auto &t : T) {
        //t.join();
    //}
    //ASSERT(v.v == nth * nit, "Invalid count", v.v);
//}

int test() {
    TEST(test_alloc).run();
    TEST(test_many_inc)
        .benchmark(50, 1, 1e6)
        .benchmark(50, 10, 1e6);
    TEST(test_many_alloc)
        .benchmark(50, 1, 1e6)
        .benchmark(50, 10, 1e6);
    return 0;
}

