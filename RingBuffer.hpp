#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include <cstring>
#include <cstddef>

template <typename T, size_t CAPACITY>
class RingBuffer
{
    T buffer[CAPACITY];
    std::atomic<size_t> head{0};
    std::atomic<size_t> tail{0};

public:
    RingBuffer() = default;

    bool push(const T* data, size_t size) {
        size_t current_head = head.load(std::memory_order_relaxed);
        size_t current_tail = tail.load(std::memory_order_acquire);

        if (size > CAPACITY - (current_head - current_tail)) return false;

        for (size_t i = 0; i < size; i++) {
            buffer[(current_head + i) % CAPACITY] = data[i];
        }

        head.store(current_head + size, std::memory_order_release);
        return true;
    }

    size_t pop(T* dest, size_t size) {
        size_t current_head = head.load(std::memory_order_relaxed);
        size_t current_tail = tail.load(std::memory_order_acquire);

        size_t available = current_head - current_tail;
        size_t read_able = size < available ? size : available;

        if (read_able == 0) return 0;

        for (size_t i = 0; i < read_able; i++) {
            dest[i] = buffer[(current_tail + i) % CAPACITY];
        }

        tail.store(current_tail + read_able, std::memory_order_release);
        return read_able;
    }

    size_t size() const{
        return head.load(std::memory_order_relaxed) - tail.load(std::memory_order_relaxed);
    }

    ~RingBuffer() = default;
    
};