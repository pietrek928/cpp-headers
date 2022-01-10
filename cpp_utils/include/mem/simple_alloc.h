#ifndef __SIMPLE_ALLOC_H_
#define __SIMPLE_ALLOC_H_

#include <utils/utils.h>

#include <exception>
#include <sys/mman.h>


class SimpleAllocator {
    constexpr static int PAGE_SIZE = 4096;

    void *ptr = MAP_FAILED;
    size_t size;

    static auto align_size(size_t size) {
        return (size + PAGE_SIZE - 1) & ((size_t)~(PAGE_SIZE - 1));
    }

    public:

    SimpleAllocator(size_t size=size_t(1e9+9))
        : size(align_size(size)) {
        ptr = mmap(
            NULL, size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0
        );
        ASSERT_EXC_VOID(ptr != MAP_FAILED, std::bad_alloc
            //"mmap of", size, "bytes failed"
        );
    }

    SimpleAllocator(SimpleAllocator &&a) {
        ptr = a.ptr;
        a.ptr = MAP_FAILED;

        size = a.size;
    }

    SimpleAllocator &operator=(SimpleAllocator &&a) {
        clear();
        ptr = a.ptr;
        a.ptr = MAP_FAILED;

        size = a.size;

        return *this;
    }

    auto get_data() {
        return ptr;
    }

    template<bool can_move = false>
    void resize(size_t new_size) {
        new_size = align_size(new_size);
        if (likely(size != new_size)) {
            auto new_ptr = mremap(
                ptr, size, new_size, can_move ? MREMAP_MAYMOVE : 0
            );

            ASSERT_EXC_VOID(new_ptr != MAP_FAILED, std::bad_alloc
                //"mremap of", size, "to", new_size, "failed"
            );

            ptr = new_ptr;
            size = new_size;
        }
    }

    void clear() {
        if (likely(ptr != MAP_FAILED)) {
            munmap(ptr, size);
            ptr = MAP_FAILED;
        }
    }

    ~SimpleAllocator() {
        clear();
    }
};

#endif /* __SIMPLE_ALLOC_H_ */

