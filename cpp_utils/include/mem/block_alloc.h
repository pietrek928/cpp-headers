#ifndef __BLOCK_ALLOC_H_
#define __BLOCK_ALLOC_H_

#include <unordered_map>
#include <set>

#include "lock_pool.h"
#include "simple_alloc.h"

template<class Obj, class Tl>
class UseHolder {
    Obj &o;
    decltype(((Tl*)NULL)->lock()) l;

    INLINE_WRAPPER
    operator Obj&() {
        return o;
    }

    public:
    INLINE_WRAPPER
    Obj &obj() {
        return o;
    }

    INLINE_WRAPPER
    UseHolder(Obj &o, Tl &lock)
        : o(o), l(lock.lock()) {}
};

#pragma pack(push, 1)
/* Single-owner reference holder */
template<class Obj>
class SingleOwnerRefHolder {
    using idx_t = decltype(Obj::allocator.idx_type_obj);

    idx_t id;

    public:

    INLINE_WRAPPER
    SingleOwnerRefHolder(idx_t id) : id(id) {}

    INLINE_WRAPPER
    auto use() {
        return Obj::allocator.use(id);
    }

    INLINE_WRAPPER
    ~SingleOwnerRefHolder() {
        Obj::allocator.delete_(id);
    }
};
/*
 * TODO: multi-owner reference
 * */
#pragma pack(pop)

template<class Obj, class BufferAllocator = SimpleAllocator>
class BlockAlloc : public LockObject {
    public:
        using idx_t = unsigned int;
        constexpr static idx_t idx_type_obj = 0;

    private:

    idx_t size = 0;
    std::set<idx_t, std::greater<idx_t>> free_blocks;

    PoolLock<idx_t, 16> lock_pool;

    BufferAllocator buffer;

    // TODO: align ?
    auto calc_size(idx_t size) {
        return size * sizeof(Obj);
    }

    INLINE_WRAPPER
    Obj *buf_ptr() {
        return (Obj*)buffer.get_data();
    }

    void reduce_free() {
        auto it = free_blocks.begin();
        while (it != free_blocks.end() && *it == size-1) {
            it = free_blocks.erase(it);
            size --;
        }
    }

    bool should_reposition(idx_t i) {
        if (free_blocks.empty()) {
            return false;
        }
        return i >= size - free_blocks.size();
    }

    auto reposition_obj(idx_t i) {
        auto new_pos_it = free_blocks.rbegin();
        if (new_pos_it == free_blocks.rend()) {
            return i;
        }
        auto new_pos = *new_pos_it;
        if (new_pos > i) {
            return i;
        }

        buf_ptr()[new_pos] = std::move(buf_ptr()[i]);

        free_blocks.erase(-- new_pos_it.base());
        free_blocks.insert(i);
        reduce_free();
        return new_pos;
    }

    void delete_obj(idx_t i) {
        auto deleted_ptr = buf_ptr() + i;
        deleted_ptr->~Obj();

        free_blocks.insert(i);
    }

    template<class ... Targs>
    idx_t emplace_obj(Targs& ...constructor_args) {
        auto new_pos_it = free_blocks.rbegin();
        if (new_pos_it == free_blocks.rend()) {
            auto new_pos = size ++; // TODO: check size left
            new (buf_ptr() + new_pos) Obj(constructor_args...);

            return new_pos;
        } else {
            auto new_pos = *new_pos_it;
            new (buf_ptr() + new_pos) Obj(constructor_args...);
            free_blocks.erase(-- new_pos_it.base());

            return new_pos;
        }
    }

    public:

    BlockAlloc(idx_t size=16)
        : size(size), buffer() {
    }

    template<class ... Targs>
    auto emplace(Targs& ...constructor_args) {
        auto l = lock();
        return SingleOwnerRefHolder<Obj>(
            emplace_obj(constructor_args...)
        );
    }

    void delete_(idx_t i) {
        auto l = lock();
        auto lo = lock_pool.lock(i);
        delete_obj(i);
    }

    auto reposition(idx_t i) {
        if (!should_reposition(i)) {
            return i;
        }
        auto l = lock();
        auto lo = lock_pool.lock(i);
        return reposition_obj(i);
    }

    auto use(idx_t i) {
        return UseHolder(
            buf_ptr()[i], lock_pool.get_locker(i)
        );
    }
};

#endif /* __BLOCK_ALLOC_H_ */

