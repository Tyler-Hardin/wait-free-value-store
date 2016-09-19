# wait-free-value-store
Wait free, SPMC value store in C++14

More specifically, reads are wait free population oblivious and writes are wait free bounded.

Depends on `boost::optional` and `boost::shared_array`.

Compile with `g++ -std=c++14 example.cpp -lpthread`.

Example:
```c++
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

int main() {
    std::thread r1(read_test), r2(read_test);
    write_test();
    r1.join();
    r2.join();
    return 0;
}
```
