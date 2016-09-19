#ifndef BLOCKING_READER_WRITER_STORE_HPP
#define BLOCKING_READER_WRITER_STORE_HPP

#include <atomic>
#include <mutex>

/**
 * A naive mutex protected reader-writer value store.
 */
template<typename T>
class blocking_reader_writer_store {
    std::atomic<int> num_readers{0};
    std::mutex rw_mutex;
    T data;

public:
    class reader_view {
        friend class blocking_reader_writer_store;
        blocking_reader_writer_store* parent;

        reader_view(blocking_reader_writer_store* parent) : parent(parent) {
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
            std::lock_guard<std::mutex> lk(parent->rw_mutex);
            return parent->data;
        }
    };

    blocking_reader_writer_store(T init) {
        data = init;
    }

    ~blocking_reader_writer_store() {
        std::lock_guard<std::mutex> lk(rw_mutex);
    }

    /**
     * Returns a reader_view for accessing the store.
     */
    const reader_view get_view() {
        return reader_view(this);
    }

    /**
     * Write a new value.
     */
    void write(T data) {
        std::lock_guard<std::mutex> lk(rw_mutex);
        this->data = data;
    }
};

#endif
