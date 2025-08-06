#pragma once


#include <cstdint>
#include <cstddef>
#include <vector>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>


#include "blockingqueue.hpp"


/**
 * IO intensive programs uses multithread programming, 
 *      on the other hand, CPU intensive programs incur extra cost of switching between threads
 * 
 */


template<uint16_t core, uint16_t max>
class ThreadPool {

    static_assert(max > core and core > 0, "max_size must be greater than core_size, both should be positive");

private:

    std::vector<std::thread> workers;
    
    const size_t qlen;
    BlockingQueue<std::function<void()>> commandList;

public:

    ThreadPool(size_t qlen) : qlen(qlen) {

    }

    ~ThreadPool() {

    }

    void exec(std::function<void()> task) {
        if (workers.size() <= core) {
            workers.push_back(std::thread([this] () -> {
                commandList.pop_front();
            }))
        } else if (worker.size() <= max) {

        }
        commandList.push_back(task);
    }

};
