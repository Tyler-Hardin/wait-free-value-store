#ifndef RING_DEQUE_HPP
#define RING_DEQUE_HPP

#include <boost/optional.hpp>
#include <boost/shared_array.hpp>

#include <cassert>

template<typename T>
class ring_deque {
    static constexpr std::size_t INIT_CAP = 16; 

    boost::shared_array<T> data{new T[INIT_CAP]};
    std::size_t _cap = INIT_CAP;
    std::size_t _size = 0;

    std::size_t bidx = 0;
    std::size_t eidx = 0;

    auto dec_idx(auto idx) {
        return idx > 0 ? idx - 1 : _cap - 1;
    }

    auto inc_idx(auto idx) {
        return (idx + 1) % _cap;
    }

    void init_idx(std::size_t idx, T&& elem) {
        new (&data[idx]) T(elem);
    }

    void fini_idx(std::size_t idx) {
        data[idx].~T();
    }

public:
    ~ring_deque() {
        auto i = front();
        while (i) {
            i->~T();
            pop_front();
            i = front();
        }
    }

    auto front() {
        boost::optional<T&> ret;
        if (_size > 0) ret = data[bidx];
        return ret;
    }

    bool pop_front() {
        bool can_pop = bidx != eidx;
        if (can_pop) {
            fini_idx(bidx);
            bidx = inc_idx(bidx);
            _size--;
        }
        return can_pop;
    }

    void push_front(T elem) {
        if (_size < _cap) {
            bidx = dec_idx(bidx);
            init_idx(bidx, std::move(elem));
            _size++;
        } else {
            reserve(_cap * 3 / 2);
            push_front(elem);
        }
    }

    auto back() {
        boost::optional<T&> ret;
        if (_size > 0) ret = data[eidx - 1];
        return ret;
    }

    bool pop_back() {
        bool can_pop = bidx != eidx;
        if (can_pop) {
            fini_idx(eidx);
            eidx = dec_idx(eidx);
            _size--;
        }
        return can_pop;
    }

    void push_back(T elem) {
        if (_size < _cap) {
            init_idx(eidx, std::move(elem));
            eidx = inc_idx(eidx);
            _size++;
        } else {
            reserve(_cap * 3 / 2);
            push_back(elem);
        }
    }

    void clear() {
        *this = ring_deque();
    }

    bool empty() {
        return _size == 0;
    }

    std::size_t size() {
        return _size;
    }

    void reserve(std::size_t new_cap) {
        assert(new_cap > _size);
        boost::shared_array<T> new_data{new T[new_cap]};
        std::size_t idx = bidx;
        for (std::size_t i = 0;i < _size;i++) {
            new (&new_data[i]) T(std::move(data[idx]));
            idx = inc_idx(idx);
            assert(i < new_cap);
        }
        data = new_data;
        bidx = 0;
        eidx = _size;
        _cap = new_cap;
    }
};

#endif
