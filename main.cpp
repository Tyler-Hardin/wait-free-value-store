#include <atomic>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

template<typename Fn, typename... Args>
concept bool Callable = requires(Fn f, Args... args) { f(args...); };

/**
 * Wait-free reader-writer value store.
 *
 * Gotchas: Reading threads (including the writer if it's also going to read)
 * must call init_reader() before any threads read. They must also call 
 * fini_reader().
 */
template<typename T, std::size_t NumReaders>
class reader_writer_store {
    struct node {
        T data;
        node* next;
        long long seq_num;
        node(T data, long long seq) : data(data), seq_num(seq) {}
    };

    // Current node stored.
    std::atomic<uintptr_t> cur;
    
    // Sequence number of cur.
    std::atomic<long long> seq_num;

    // Sequence numbers of all readers, by thread id.
    std::map<std::thread::id, std::atomic<long long>> seq_numbers;
    std::mutex map_write_mutex;
    
    // Used to ensure invariants all threads init before reading and fini
    // before deconstruction.
    std::atomic<int> readers_inited{0};

    // Nodes that were read from. (Others are deleted immediately.)
    node* head;
    node* tail;


    static constexpr uintptr_t READ_BIT = 1;
    static constexpr uintptr_t PTR_MASK = ~READ_BIT;

    void cleanup() {
        // Find the lowest sequence number that threads started reading.
        long long lowest_seq_num = seq_num.load();
        for (const auto& i : seq_numbers) {
            if (i.second < lowest_seq_num) {
                lowest_seq_num = i.second;
            }
        }

        // GC nodes with lower sequence numbers.
        while (head != tail && head->seq_num < lowest_seq_num) {
            auto next = head->next;
            delete head;
            head = next;
        }
    }

public:
    reader_writer_store(T init) {
        node* init_data = new node(init, 0);
        head = init_data;
        tail = init_data;
        cur.store((uintptr_t)init_data);
    }

    ~reader_writer_store() {
        uintptr_t temp = cur.exchange(0);

        // Wait for all threads to call fini.
        while(readers_inited.load() > 0);
        delete (node*)(temp & PTR_MASK);
        cleanup();
    }

    void init_reader() {
        {
            std::lock_guard<std::mutex> lk(map_write_mutex);
            seq_numbers.emplace(std::this_thread::get_id(), 0);
        }
        readers_inited++;
        while(readers_inited.load() < NumReaders);
    }

    void fini_reader() {
        seq_numbers[std::this_thread::get_id()] = seq_num.load();

        readers_inited--;
        while(readers_inited.load() > 0);
    }

    template<typename Fn>
    requires Callable<Fn, const T&>
    void read(Fn f) {
        seq_numbers[std::this_thread::get_id()].store(seq_num.load());
        node *n = (node*)((cur |= 1) & PTR_MASK);
        f(n->data);
    }

    void write(T data) {
        // Either assume seq num never overflows or actually deal with it.
        // Might have to lock to deal with wrap around???

        node* new_data = new node(std::move(data), ++seq_num);
        uintptr_t temp = cur.exchange((uintptr_t)new_data);
        bool can_delete = !(temp & READ_BIT);
        node* old_data = (node*)(temp & PTR_MASK);

        if (can_delete) {
            // Delete nodes that were never read.
            delete old_data;
        } else {
            // Save nodes that were for GC.
            tail->next = old_data;
            tail = old_data;
        }

        // Run GC.
        cleanup();
    }
};

reader_writer_store<std::string, 2> store("0");

void read() {
    store.init_reader();
    bool done = false;
    while (!done) {
        store.read([&done](const std::string& s) {
            std::cout << (s + "\n");
            done = s == "done";
        });
    }
    store.fini_reader();
}

void write() {
    for (int i = 0;i < 100000000;i++)
        store.write(std::to_string(i));
    store.write("done");
}

int main() {
    std::thread r1(read), r2(read);
    write();
    r1.join();
    r2.join();
    return 0;
}
