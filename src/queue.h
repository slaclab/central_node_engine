#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>

template<typename T>
class Queue
{
public:
    // Convinient typedef. This is the data type returned
    // by the pop methods.
    typedef std::shared_ptr<T> value_type;

    Queue();

    // Push a nee value to the queue.
    void push(T val);

    // Blocking call. The thread will wait (sleeping) until a value
    // is available in the queue.
    value_type pop();

    // Non-blocking call. If not value is available, the call will
    // return immediatelly with a null pointer.
    value_type try_pop();

    // Get the maximum value
    std::size_t get_max_size() const;

    // Clear the internal counters.
    void clear_counters();

    // Reset the queue: clear all elements in the queue, and the
    // internal counters.
    void reset();

private:
    std::size_t             watermark;
    std::queue<T>           q;
    mutable std::mutex      m;
    std::condition_variable cv;

    // Helper function to return and pop the front element
    // from the queue, to avoid code duplication between pop()
    // and try_pop(). It is call when we are sure there is at
    // least one element in the queue.
    value_type pop_and_get();

    // Helper predicate function used for the conditional variable
    // wait methods. We need this as our old rhel6 host don't
    // support C++11 , otherwise we could have used lambda expressions.
    bool pred() { return !q.empty(); }
};

#endif

