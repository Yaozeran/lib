#pragma once


#include <array>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>


template<uint32_t core_size, uint32_t max_size>
class ThreadPool {

    static_assert(max_size > core_size, "max_size must be greater than core_size");
    
    static_assert(core_size > 0, "core_size must be greater than 0");

private:

    final uint32_t queue_size;

    std::array<std::thread, core_size> core;
    std::array<std::thread, max_size - core_size> support;

    std::vector<std::function<void()>> tasks;

    std::mutex task_mutex; // protect blocking queue tasks
    std::condition_variable cv;

public:

    ThreadPool(uint32_t queue_size) : queue_size(queue_size) {
        for (size_t i = 0; i < core_size; ++i) {
            core.emplace_back()
        }
    }

    ~ThreadPool() {

    }

    std::thread* create_core_thread() {
        
    }

    void exec(std::function<void()> task) {
        task.push_back(task);
    }

};

