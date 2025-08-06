#pragma once


#include <cstdint>
#include <cstddef>
#include <array>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>


#include "blockingqueue.hpp"


template<uint16_t core, uint16_t max>
class ThreadPool {

    static_assert(max > core and core > 0, "max_size must be greater than core_size, both should be positive");

private:

    std::array<std::thread, max> workers;
    
    BlockingQueue<std::function<void()>> commandList;

public:

    ThreadPool() {

    }

    ~ThreadPool() {

    }

    void exec(std::function<void()> task) {
        commandList.push_back(task);
    }

};
