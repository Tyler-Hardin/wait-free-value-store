#ifndef READER_WRITER_STORE_HPP
#define READER_WRITER_STORE_HPP

#include "ring_deque.hpp"

#include <atomic>

/**
 * Wait-free SPMC reader-writer value store.
 */
template<typename T>
class reader_writer_store {
    static constexpr uintptr_t READ_BIT = 1;
    static constexpr uintptr_t PTR_MASK = ~READ_BIT;

    // Current node to read from.
    std::atomic<uintptr_t> cur;
    
    // Need to know how many readers there are because that bounds the number
    // of outstanding reads.
    std::atomic<int> num_readers{0};

    // Queue for reclaiming the oldest element of the cache. (Which we can know
    // doesn't have an outstanding read.) Ring buffer for better data locality.
    ring_deque<T*> gc_queue;

    // Lock to ensure single writer. Shouldn't ever actually function as a
    // lock, as long as API invariants are met. (I.e. single producer.)
    std::atomic<bool> write_lock;

public:
    class reader_view {
        friend class reader_writer_store;
        reader_writer_store* parent;

        reader_view(reader_writer_store* parent) : parent(parent) {
            parent->num_readers++;
        }

    public:
        ~reader_view() {
            parent->num_readers--;
        }

        /**
         * Read the store.
         */
        const T& operator*() const {
            return *(T*)((parent->cur |= 1) & PTR_MASK);
        }
    };

    reader_writer_store(T init) {
        static_assert(sizeof(T) > 1);
        cur.store((uintptr_t) new T{std::move(init)});
        assert(cur.load() % 2 == 0);
    }

    ~reader_writer_store() {
        uintptr_t temp = cur.exchange(0);

        // Wait for all views to destruct.
        while(num_readers.load() > 0);
        delete (T*)(temp & PTR_MASK);
        while (!gc_queue.empty()) {
            delete *gc_queue.front();
            gc_queue.pop_front();
        }
    }

    /**
     * Returns a reader_view for accessing the store.
     */
    const reader_view get_view() {
        return reader_view(this);
    }

    /**
     * Write a new T instance.
     * Only safe for one thread.
     */
    void write(T data) {
        T* new_data = nullptr;

        // Lock, just in case. Shouldn't actually function as a lock.
        bool expected = false;
        assert(write_lock.compare_exchange_weak(expected, true));

        if (gc_queue.size() <= num_readers.load()) {
            new_data = new T{std::move(data)};
            assert((uintptr_t)new_data % 2 == 0);
        } else {
            while (gc_queue.size() > num_readers.load() + 2) {
                delete *gc_queue.front();
                gc_queue.pop_front();
            }

            new_data = *gc_queue.front();
            new_data->~T();
            new (new_data) T{std::move(data)};
            gc_queue.pop_front();
        }

        uintptr_t temp = cur.exchange((uintptr_t)new_data);
        bool can_delete = !(temp & READ_BIT);
        T* old_data = (T*)(temp & PTR_MASK);

        if (can_delete) {
            // This node doesn't have any outstanding reads. Save it for
            // immediate reuse.
            gc_queue.push_front(old_data);
        } else {
            // This one might have outstanding reads. Save it in the queue
            // and wait until we can know it's finished. We'll know it is
            // when it gets back to the front of the queue, as there can
            // only be `num_readers` outstanding reads.
            gc_queue.push_back(old_data);
        }
        write_lock.store(false);
    }
};

#endif
