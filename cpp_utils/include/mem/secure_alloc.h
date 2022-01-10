#ifndef __SECURE_ALLOC_H_
#define __SECURE_ALLOC_H_

#include <utils/utils.h>
#include <mem/simple_alloc.h>

#include <sys/mman.h>
#include <atomic>
#include <vector>

template<int PAGE_SIZE=4096>
class SecureAllocator {
    void *ptr = MAP_FAILED;
    size_t size;
    
    static void swipe_data(void *_data, size_t data_size) {
        uint32_t *data = (uint32_t*)_data;
        uint32_t *data_end = (uint32_t*)(((char*)_data) + data_size);
        std::fill(data, data_end, 0xFFFFFFFF);
        std::atomic_thread_fence(std::memory_order_release);
        std::fill(data, data_end, 0);
        std::atomic_thread_fence(std::memory_order_release);
    }

    static auto align_size(size_t size) {
        return (size + PAGE_SIZE - 1) & ((size_t)~(PAGE_SIZE - 1));
    }

    public:

    SecureAllocator(size_t size=size_t(PAGE_SIZE))
        : size(align_size(size)) {
        ptr = mmap(
            NULL, size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_LOCKED, -1, 0
        );
        ASSERT_EXC_VOID(ptr != MAP_FAILED, std::bad_alloc
            //"mmap of", size, "bytes failed"
        );
    }

    SecureAllocator(SimpleAllocator &&a) {
        ptr = a.ptr;
        a.ptr = MAP_FAILED;

        size = a.size;
    }

    SecureAllocator &operator=(SecureAllocator &&a) {
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
            if (new_size < size) {
                swipe_data(((char*)ptr)+new_size, size-new_size);
            }
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
            swipe_data(ptr, size);
            munmap(ptr, size);
            ptr = MAP_FAILED;
        }
    }

    ~SecureAllocator() {
        clear();
    }
};

#endif /* __SECURE_ALLOC_H_ */
