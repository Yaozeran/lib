#pragma once


#include <vector>
#include <mutex>
#include <condition_variable>
#include <functional>


/**
 * A blocking queue implementation using vector.
 */
template<typename T>
class BlockingQueue {

private:

    std::vector<T> queue; 

    std::mutex mutex;

    std::condition_variable cv;

public:

    BlockingQueue(/* args */) {
    }
    
    ~BlockingQueue() {
    }

    void push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push_back(item);
        cv.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex); // auto unlock when lock goes out of scope
        cv.wait(lock, [] { return !queue.empty(); }) // block thread until an item is available
        T item = queue.front();
        queue.erase(queue.begin());
        return item;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        queue.clear();
    }

    void waitUntilEmpty() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this] { return queue.empty(); });
    }
};

