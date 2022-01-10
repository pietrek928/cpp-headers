#ifndef __LOCK_SET_OBJ_H_
#define __LOCK_SET_OBJ_H_

#include <unordered_set>

#include "lock.h"

template<class To>
class SetLock : LockObject {
    std::unordered_set<To> lock_set;

    public:
        void lock_c(To &v) {
            auto l = LockObject::lock();
            while (lock_set.find(v) != lock_set.end()) {
                wait();
            }
            lock_set.insert(v);
        }

        void unlock_c(To &v) {
            { // scope for lock
                auto l = LockObject::lock();
                lock_set.erase(v);
            }
            notifyAll(); // All ?
        }

        INLINE_WRAPPER
        auto lock(To &v) {
            return ScopeLockArg(*this, v);
        }
};

#endif /* __LOCK_SET_OBJ_H_ */

