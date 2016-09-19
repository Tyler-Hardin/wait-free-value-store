#include "blocking_reader_writer_store.hpp"
#include "reader_writer_store.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

constexpr std::size_t NUM_READERS = 5;

template<typename Store>
void test(Store& store){
    auto read_test = [&store]() {
        auto view = store.get_view();
        bool done = false;
        while (!done) {
            const std::string& s = *view;
            //std::cout << (s + "\n");
            done = s == "done";
        }
    };

    auto write_test = [&store]() {
        for (int i = 0;i < 10000000;i++)
            store.write(std::to_string(i));
        store.write("done");
    };

    auto start = std::chrono::system_clock::now();
    std::vector<std::thread> threads;
    for (int i = 0;i < NUM_READERS;i++)
        threads.emplace_back(read_test);
    write_test();
    for (auto& i : threads) i.join();
    auto end = std::chrono::system_clock::now();
    std::cout << (end - start).count() << std::endl;
}

int main() {
    blocking_reader_writer_store<std::string> blocking_store("0");
    reader_writer_store<std::string> store("0");
    std::cout << "Blocking: ";
    test(blocking_store);
    std::cout << "Wait free: ";
    test(store);
    return 0;
}
