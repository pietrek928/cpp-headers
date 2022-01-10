#ifndef __POOL_LOCK_H_
#define __POOL_LOCK_H_

#include "lock.h"

template<class To, int N>
class PoolLock {
    LockObject L[N];

    inline int hash(To &v) {
        return (v * 74675675667) % N;
    }

    public:
        auto &get_locker(To &v) {
            auto h = hash(v);
            return L[h];
        }

        auto lock(To &v) {
            return ScopeLock(
                get_locker(v)
            );
        }
};

#endif /* __POOL_LOCK_H_ */

