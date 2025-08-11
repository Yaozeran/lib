/**
 * scheduled tasks are executed at the end of a tick, rather than executed precisely according to their presetted delay
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

#include "../concurrent/threadpool.hpp"


namespace timewheel
{

    struct Task {
        int id{0};
        size_t life{0};
        std::shared_ptr<std::function<void()>> task{nullptr};
    };
    
} // namespace timewheel


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

    using Task = timewheel::Task;

private:

    const size_t size;
    const Rep tick; // the duration of a tick

    std::vector<std::vector<Task>> slots;
    std::atomic<size_t> nextid;
    std::atomic<size_t> idx;

    std::unique_ptr<std::thread> ticker; // simulate tick

    std::mutex mtx;

    std::shared_ptr<FixedThreadPool> core;

    std::atomic<bool> running;

public:

    TimeWheel(size_t s, Rep t, std::shared_ptr<FixedThreadPool> ftp)
    : size(s), tick(t), core(std::move(ftp)), slots(s), nextid(0), idx(0), running(true) {
        ticker = std::make_unique<std::thread>(&TimeWheel::tickerfunc, this);
    }

    ~TimeWheel() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            running = false;
        }
    }

    size_t Appoint(Rep delay, std::shared_ptr<std::function<void()>> f) {
        size_t ticks = delay / tick;
        size_t life = ticks / size;
        size_t slot = (idx + ticks % size) % size;
        // generate task
        Task task = Task();
        task.id = nextid.fetch_add(1);
        task.life = life;
        task.task = std::move(f);
        {
            std::unique_lock<std::mutex> lock(mtx);
            slots[i].push_back(task);
        }
        return task.id;
    }

private:

    void tickerfunc() {
        /**
         * exec tasks within a tick interval at the end of the tick, because need to wait until all tasks are properly
         * pushed into the task queue of that tick
         */
        clock::time_point tp = clock::now();
        std::vector<std::function<void()>> todos;
        // inif loop
        while (true) {
            // tick
            tp += duration(tick);
            std::this_thread::sleep_until(tp);
            // reset
            todos = {};
            // extract tasks
            {
                std::unique_lock<std::mutex> lock(mtx);
                std::vector<Task>& bucket = slots[idx];
                auto ptr = bucket.begin();
                while (ptr != bucket.end()) {
                    if (ptr->life == 0) {
                        todo.push_back(ptr->task);
                        ptr = bucket.erase(ptr);
                    } else {
                        ptr->life--;
                        ++ptr;
                    }
                }
            }
            // execute
            for (std::function<void()>& task : todos) {
                core->Exec(task);
            }
            // proceed to next slot, atomic
            idx = (idx + 1) % size;
        }
    }

};
