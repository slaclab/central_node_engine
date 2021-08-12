#include "queue.h"

template<typename T>
Queue<T>::Queue()
:
    watermark(0)
{
}

template<typename T>
void Queue<T>::push(T val)
{
    {
        std::lock_guard<std::mutex> lock(m);
        q.push(std::move(val));

        if (q.size() > watermark)
            watermark= q.size();
    }
    cv.notify_one();
}

template<typename T>
void Queue<T>::reset()
{
    std::lock_guard<std::mutex> lock(m);
    std::queue<T>().swap(q);
    watermark = 0;
}

template<typename T>
typename Queue<T>::value_type Queue<T>::pop()
{
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, std::bind(&Queue::pred, this));
    return std::make_shared<T>( pop_and_get() );
}

template<typename T>
void Queue<T>::pop(T& val)
{
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, std::bind(&Queue::pred, this));
    val = pop_and_get();
}

template<typename T>
typename Queue<T>::value_type Queue<T>::try_pop()
{
    std::lock_guard<std::mutex> lock(m);
    if (q.empty())
        return value_type();
    return std::make_shared<T>( pop_and_get() );
}

template<typename T>
T Queue<T>::pop_and_get()
{
    T val ( std::move(q.front()) );
    q.pop();
    return val;
}

template<typename T>
std::size_t Queue<T>::get_max_size() const
{
    std::lock_guard<std::mutex> lock(m);
    return watermark;
}

template<typename T>
void Queue<T>::clear_counters()
{
    std::lock_guard<std::mutex> lock(m);
    watermark = 0;
}

template class Queue< std::vector<uint8_t> >;
template class Queue< std::vector<uint32_t> >;

