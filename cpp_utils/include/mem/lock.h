#ifndef __LOCK_OBJ_H_
#define __LOCK_OBJ_H_

#include <utils/utils.h>

template<class Tm, class Targ, bool movable = false>
class ScopeLockArg { // TODO: move only object
    Tm *m;
    Targ arg;

    inline void clear() {
        if (!movable || likely(m)) {
            m->unlock_c(arg);
        }
    }

    public:
        ScopeLockArg(ScopeLockArg &&l)
            : m(l->m), arg(l->arg) {
            ASSERT(movable, "Lock is not movable");
            l->m = NULL;
        }

        void operator=(ScopeLockArg &&l) {
            ASSERT(movable, "Lock is not movable");
            clear();
            m = l->m;
            arg = l->arg;
            l->m = NULL;
        }

        INLINE_WRAPPER
        ScopeLockArg(Tm &_m, Targ arg)
            : m(&_m), arg(arg) {
            m->lock_c(arg);
        }

        INLINE_WRAPPER
        ~ScopeLockArg() {
            m->unlock_c(arg);
        }
};

template<class Tm, bool movable = false>
class ScopeLock { // TODO: move only object
    Tm *m;

    INLINE_WRAPPER
    void clear() {
        if (!movable || likely(m)) {
            m->unlock_c();
        }
    }

    public:
        ScopeLock(ScopeLock &&l) : m(l->m) {
            ASSERT(movable, "Lock is not movable");
            l->m = NULL;
        }

        void operator=(ScopeLock &&l) {
            ASSERT(movable, "Lock is not movable");
            clear();
            m = l->m;
            l->m = NULL;
        }

        INLINE_WRAPPER
        ScopeLock(Tm &_m)
            : m(&_m) {
            m->lock_c();
        }

        INLINE_WRAPPER
        ~ScopeLock() {
            clear();
        }
};

#ifndef USE_PTHREAD

#include <climits>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>

#define COUNT_BRUTE_WAITERS
#define BRUTE_WAIT_LIMIT 24
#define BRUTE_WAITERS_COUNT 1

/*
 * Faster for rarely conflicts ( typical case )
 * Lower memory footprint
 * */
class LockObject {
    using futex_t = int;

    futex_t _futex_var = 0;
    int num_futex_waiters = 0;

#ifdef COUNT_BRUTE_WAITERS
    int num_brute_waiters = 0;
#else /* COUNT_BRUTE_WAITERS */
    constexpr static int num_brute_waiters = 0;
#endif /* COUNT_BRUTE_WAITERS */

    inline int futex(
        int futex_op, futex_t val,
        const struct timespec *timeout, int *uaddr2, int val3
    ) {
        return syscall(
            SYS_futex, &_futex_var, futex_op, val,
            timeout, uaddr2, val3
        );
    }

    inline void yield() {
        syscall(SYS_sched_yield);
    }

    inline auto add_to_futex_waiters(int n) {
        return __sync_add_and_fetch(&num_futex_waiters, n);
    }

    inline auto add_to_brute_waiters(int n) {
#ifdef COUNT_BRUTE_WAITERS
        return __sync_add_and_fetch(&num_brute_waiters, n);
#else /* COUNT_BRUTE_WAITERS */
        return num_brute_waiters;
#endif /* COUNT_BRUTE_WAITERS */
    }

    inline void futex_wait(futex_t start_val) {
        add_to_futex_waiters(1);
        futex(FUTEX_WAIT, start_val, NULL, NULL, 0);
        add_to_futex_waiters(-1);
    }

    inline auto atomic_try_lock() {
        return __sync_bool_compare_and_swap(&_futex_var, 0, 1);
    }

    inline bool __brute_lock_wait(int limit=-1) {
        add_to_brute_waiters(1);
        do {
            if (atomic_try_lock()) {
                add_to_brute_waiters(-1);
                return true;
            }
            yield();
        } while (--limit);

        add_to_brute_waiters(-1);
        return false;
    }

    void __lock_wait() {
        if (
            num_brute_waiters < BRUTE_WAITERS_COUNT
            && __brute_lock_wait(BRUTE_WAIT_LIMIT)
        ) {
            return;
        }
        while (!atomic_try_lock()) {
            futex_wait(1);
        };
    }

    public:
    using lock_holder_t = ScopeLock<LockObject>;

    // MUST be used inside synchronized block
    void wait() {
        ASSERT_DBG(locked(), "Object MUST be locked to use wait()");
        unlock_c();
        futex_wait(0);
        lock_c();
    }

    auto notify(int n=1) {
        if (num_futex_waiters) {
            return futex(FUTEX_WAKE, n, NULL, NULL, 0);
        }
        return 0;
    }

    void notifyAll() {
        notify(INT_MAX);
    }

    bool locked() {
        return (bool)_futex_var;
    }

    void lock_c() {
        if (unlikely(!atomic_try_lock())) {
            __lock_wait();
        }
    }

    void unlock_c() {
        __sync_lock_release(&_futex_var);
        if (!num_brute_waiters) {
            notify();
        }
    }

    INLINE_WRAPPER
    auto lock() {
        return ScopeLock(*this);
    }
};

#else /* ! USE_PTHREAD */

#include <pthread.h>

/* compatibility pthread version */
class LockObject {
    pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

    public:

    // MUST be used inside synchronized block
    void wait() {
        ASSERT_DBG(locked(), "Object MUST be locked to use wait()");
        pthread_cond_wait(&cond, &locker);
    }

    void notify(int n=1) {
        pthread_cond_signal(&cond);
    }

    void notifyAll() {
        pthread_cond_signal(&cond);
    }

    bool locked() {
        if (!pthread_mutex_trylock(&locker)) {
            pthread_mutex_unlock(&locker);
            return false;
        }
        return true;
    }

    void lock_c() {
        pthread_mutex_lock(&locker);
    }

    void unlock_c() {
        pthread_mutex_unlock(&locker);
    }

    INLINE_WRAPPER
    auto lock() {
        return ScopeLock(*this);
    }
};

#endif /* ! USE_PTHREAD */

#endif /* __LOCK_OBJ_H_ */

