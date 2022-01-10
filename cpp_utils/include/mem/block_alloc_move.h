#ifndef __BLOCK_ALLOC_MOVE_H_
#define __BLOCK_ALLOC_MOVE_H_

#include <unordered_ma>
#include "simple_alloc.h"

template<class Obj, class BufferAlloc = SimpleAlloc>
class BlockAllocMove {
    using idx_t = unsigned int;

    size_t size;
    unordered_map<idx_t, idx_t> move_map;

    idx_t use(idx_t id) {
        // TODO: lock, scope lock object ?
        auto new_id_it = move_map.find(id);
        if (unlikely(new_id_it != move_map.end())) {
            id = new_id_it->second;
        }
        //
    }

    void remove(idx_t) {
    }
};

#endif /* __BLOCK_ALLOC_MOVE_H_ */

