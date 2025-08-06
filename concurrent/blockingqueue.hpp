#pragma once


#include <queue>
#include <mutex>
#include <condition_variable>


template<typename T>
class BlockingQueue {

private:

    std::queue<T> container;

    std::mutex mutex;
    std::condition_variable cond;

public:

    BlockingQueue() {

    }

    ~BlockingQueue() {

    }

    void push_back(T item) {
        std::lock_guard<std::mutex> lock(mutex);
        container.push(item);
    }

    T pop_front() {
        std::unique_lock<std::mutex> lock(mutex);
        return container.pop();
    }
};