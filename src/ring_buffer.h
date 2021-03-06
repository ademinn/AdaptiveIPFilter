#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>

template <typename T>
class ring_buffer
{
    public:
        static const size_t BUFFER_SIZE = 10;

        ring_buffer(size_t buffer_size = BUFFER_SIZE);
        ~ring_buffer();

        void put(const T&);
        T& pop();

        bool is_full() const;
        bool is_empty() const;
    private:
        size_t next(size_t) const;

        size_t buffer_size;
        T *values;

        std::atomic<size_t> first;
        std::atomic<size_t> last;
        std::atomic<size_t> count;

        std::mutex read_mutex;
        std::mutex write_mutex;
        std::condition_variable read_condition;
        std::condition_variable write_condition;
};


template <typename T>
ring_buffer<T>::ring_buffer(size_t buffer_size)
    : buffer_size(buffer_size), first(0), last(0), count(0)
{
    values = new T[buffer_size];
}


template <typename T>
ring_buffer<T>::~ring_buffer()
{
    delete[] values;
}


template <typename T>
void ring_buffer<T>::put(const T& value)
{
    std::unique_lock<std::mutex> lock(write_mutex);
    write_condition.wait(lock, [this](){ return !(this->is_full()); });
    values[last] = value;
    last = next(last);
    count++;
    read_condition.notify_one();
}


template <typename T>
T& ring_buffer<T>::pop()
{
    std::unique_lock<std::mutex> lock(read_mutex);
    read_condition.wait(lock, [this](){ return !(this->is_empty()); });
    T& value = values[first];
    first = next(first);
    count--;
    write_condition.notify_one();
    return value;
}


template <typename T>
bool ring_buffer<T>::is_full() const
{
    return count == buffer_size;
}


template <typename T>
bool ring_buffer<T>::is_empty() const
{
    return count == 0;
}


template <typename T>
size_t ring_buffer<T>::next(size_t current) const
{
    return (current + 1) % buffer_size;
}
