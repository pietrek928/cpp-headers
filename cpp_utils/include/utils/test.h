#ifndef __TEST_H_
#define __TEST_H_

#include <cstdint>
#include <time.h>
#include <sys/times.h>
#include <unistd.h>
#include <algorithm>

#include "utils.h"

constexpr auto CHANNEL_LOG_LEVEL_test = LogLevel::DEBUG;

template<class ... Targs>
class TestRunnerClass {
    const char *name;
    void(*f)(Targs...);

    struct benchmark_t {
        clock_t real_time, user_time, sys_time;
    };

    uint64_t get_real_time() {
        struct timespec tm;
        clock_gettime(CLOCK_MONOTONIC, &tm);
        return ((uint64_t)tm.tv_sec) * 1000000000 + tm.tv_nsec;
    }

    auto get_cpu_times() {
        benchmark_t r;
        struct tms tt;
        r.real_time = times(&tt);
        r.user_time = tt.tms_utime;
        r.sys_time = tt.tms_stime;
        return r;
    }

    auto run_timing(Targs... args) {
        auto start_t = get_cpu_times();
        f(args...);
        auto end_t = get_cpu_times();
        end_t.real_time -= start_t.real_time;
        end_t.user_time -= start_t.user_time;
        end_t.sys_time -= start_t.sys_time;
        return end_t;
    }

    template<class T>
    auto average_time(std::vector<T> &A) {
        auto N = A.size();
        std::sort(A.begin(), A.end());
        int b=N*0.1, e=N-(int)(N*0.1);
        double t = 0;
        for (int i = b; i < e; i++) {
            t += A[i];
        }
        return t / (e - b);
    }

    public:

    TestRunnerClass(void(*f)(Targs...), const char *name)
        : f(f), name(name) {}

    auto &benchmark(int N, Targs... args) {
        auto ticks_count = sysconf(_SC_CLK_TCK);

        std::vector<double> R(N), U(N), S(N);
        for (int i=0; i<N; i++) {
            auto t = run_timing(args...);
            R[i] = t.real_time;
            U[i] = t.user_time;
            S[i] = t.sys_time;
        }

        INFO(test) << name << "(" << N << "x):: "
            << "real: " << average_time(R) / ticks_count << "s; "
            << "user: " << average_time(U) / ticks_count << "s; "
            << "sys: " << average_time(S) / ticks_count << "s" << LOG_ENDL;

        return *this;
    }

    auto &run(Targs... args) {
        auto ticks_count = sysconf(_SC_CLK_TCK);

        auto t = run_timing(args...);

        INFO(test) << name << " OK:: "
            << "real: " << t.real_time / ticks_count << "s; "
            << "user: " << t.user_time /ticks_count << "s; "
            << "sys: " << t.sys_time / ticks_count << "s" << LOG_ENDL;

        return *this;
    }
};

template<class ... Targs>
auto __test_class__(void(*f)(Targs...), const char *name) {
    return TestRunnerClass<Targs...>(f, name);
}

#define TEST(f) __test_class__(f, #f)

#endif /* __TEST_H_ */

