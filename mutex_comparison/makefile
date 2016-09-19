HEADERS=blocking_reader_writer_store.hpp ../reader_writer_store.hpp ../ring_deque.hpp
SOURCES=main.cpp

mutex_comparison : $(HEADERS) $(SOURCES)
	g++ -std=c++14 -o $@ -I.. -lpthread $+
