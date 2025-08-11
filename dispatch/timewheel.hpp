/**
 * 
 */


#pragma once


#include <cstddef>

#include <chrono>

#include <vector>
#include <functional>

#include <atomic>
#include <thread>
#include <memory>

#include <mutex>
#include <condition_variable>

#include "../concurrent/threadpool.hpp"


/**
 * A time wheel dispatcher implementation
 * 
 * Usage example:
 *      TimeWheel<int, std::ratio<1>> tw = TimeWheel(60, 1, ptr);
 */
template<typename Rep, typename Period>
class TimeWheel {

    using clock = std::chrono::high_resolution_clock;

    using duration = std::chrono::duration<Rep, Period>;

private:

    struct Task {
        int id;
        size_t life;
        std::shared_ptr<std::function<void()>> task;
    };

    const size_t size;
    const duration tick; // the duration of a tick

    std::vector<std::vector<Task>> slots;
    std::atomic<size_t> idx;

    std::unique_ptr<std::thread> ticker; // simulate tick

    std::mutex mtx;
    std::condition_variable cond;

    std::shared_ptr<FixedThreadPool> core;

public:

    TimeWheel(size_t s, Rep t, std::shared_ptr<FixedThreadPool> ftp)
    : size(s), tick(duration(t)), core(std::move(ftp)), slots(s), idx(0) {
        ticker = std::make_unique<std::thread>(&TimeWheel::tickerfunc, this);
    }

    ~TimeWheel() {}

    void Appoint(Rep delay, std::shared_ptr<std::function<void()>> f) {

    }

    void Cancel() {

    }

private:

    void tickerfunc() {
        clock::time_point tp = clock::now();
        while (true) { 
            {
                std::unique_lock<std::mutex> lock(mtx);
                std::vector<Task>& bucket = slots[idx];
                std::vector<Task>::iterator ptr;
                while (ptr != bucket.end()) {
                    if (ptr->life == 0) {
                        
                    }
                }
            }
            // tick and proceed to next slot
            tp += tick;
            std::this_thread::sleep_until(tp);
            idx++;
        }
    }

};
