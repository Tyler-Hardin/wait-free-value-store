#include "reader_writer_store.hpp"
#include "ring_deque.hpp"

#include <iostream>
#include <thread>

reader_writer_store<std::string> store("0");

void read_test() {
    auto view = store.get_view();
    bool done = false;
    while (!done) {
        const std::string& s = *view;
        std::cout << (s + "\n");
        done = s == "done";
    }
}

void write_test() {
    for (int i = 0;i < 1000000;i++)
        store.write(std::to_string(i));
    store.write("done");
}

void ring_test() {
    ring_deque<int> rd;
    rd.push_front(1);
    rd.pop_front();
    rd.push_front(1);
    rd.pop_back();
    for (int i = 1;i < 20;i++)
        rd.push_back(i);
    rd.clear();
    for (int i = 1;i < 20;i++)
        rd.push_front(i);
}

int main() {
    ring_test();
    std::thread r1(read_test), r2(read_test);
    write_test();
    r1.join();
    r2.join();
    return 0;
}
