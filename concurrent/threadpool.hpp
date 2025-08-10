/**
 * 
 * Common sense: 
 *      thread creation and destruction are expensive: switching to kernel mode
 *      thread stack takes large space of memory: appr 8192 kbytes
 *      switching between thread context is expensive
 *      expensive to invoke large amount of threads at the same time
 * 
 * 
 * Linux: one process to create 380 threads: 3 * 1024 / 8 if program is about to take 3G
 * 
 * 
 * IO intensive programs uses multithread programming, 
 *      on the other hand, CPU intensive programs incur extra cost of switching between threads
 * 
 */


#pragma once


#include <cstddef>


#include <unordered_map>
#include <memory>
#include <thread>

#include <queue>
#include <functional>

#include <mutex>
#include <condition_variable>
#include <atomic>

#include <iostream>

#include <future>

#include <vector>
#include <chrono>


/**
 * A fixed thread pool implementation
 * 
 * @tparam core the num of long lived fixed threads in the pool
 */
template<size_t core>
class FixedThreadPool {

private:

    /**
     * some operations on the map may require the stored type to be copyable or may invalidate references, 
     * std::thread is movable but not copyable, use pointers that are copyable
     */

    std::unordered_map<int, std::unique_ptr<std::thread>> threads;
    
    std::queue<std::function<void()>> tasks;

    std::mutex mtx;
    std::condition_variable cond;

    std::atomic<bool> running;

public:

    FixedThreadPool() : running(false) {
        running = true;
        for (size_t i = 0; i < core; ++i) {
            auto threadptr = std::make_unique<std::thread>(&FixedThreadPool::threadfunc, this, i);
            threads.emplace(i, std::move(threadptr));
            std::cout << "Thread - " << i << " is created by fixed thread pool." << std::endl;
        }
    }

    ~FixedThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            running = false;
            cond.notify_all();
        }
        for (auto& [id, t]: threads) {
            if (t->joinable()) {
                t->join();
                std::cout << "Thread - " << id << " joins." << std::endl;
            }
        }
    }

    /**
     * Execute the given command by submitting it to the thread pool's task queue.
     * 
     * @param fun the runnable
     * @param args the params passed to the runnable
     */
    template<typename Func, typename... Args>
    auto Exec(Func&& fun, Args&&... args) -> std::future<std::invoke_result_t<Func, Args...>> {
        /**
         * choose invoke result instead of decltype because decltype doesn’t work well with template parameters 
         * representing generic callables, especially if those callables have overloaded or templated operator() 
         * (like generic lambdas)
         * 
         * std::forward is a utility in cpp used to perfectly forward function arguments; it preserves the value 
         * category (lvalue or rvalue) of a function argument when passing it to another function; 
         * 
         * when you pass an rvalue (like a temporary object or std::move(x)) to a function, it prefers moving the 
         * object if a move constructor or move assignment is available, then if not possible, fall back to copy
         */
        using return_type = std::invoke_result_t<Func, Args...>;
        // return default zero value when thread pool is not running
        if (!running) {
            std::cerr << "Failed to execute: fixed thread pool is no longer running." << std::endl;
            std::promise<return_type> p;
            p.set_value(return_type{});
            return p.get_future();
        }
        // package task
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<Func>(fun), std::forward<Args>(args)...)
        );
        // enqueue
        {
            std::unique_lock<std::mutex> lock(mtx);
            tasks.emplace([task] () -> void { (*task)(); }); // captures task by value, holds a copy of the shared_ptr
            cond.notify_one();
        }
        // return result
        return task->get_future();
    }

private: // helpers

    void threadfunc(int id) {
        while (true) {
            std::function<void()> task;
            // take task out of task queue
            {
                std::unique_lock<std::mutex> lock(mtx);
                cond.wait(lock, [this] () -> bool { return !running || !tasks.empty(); }); // dangling ptr of this
                if (!running) break;
                task = tasks.front();
                tasks.pop();
            }
            // exec the task
            std::cout << "FixedThread - " << id << " is executing task." << std::endl;
            task();
        }
    }
};


// /**
//  * A cached thread pool implementation
//  * 
//  * @tparam tnum the maximum number of cached threads at a specific time point
//  */
// template<size_t max>
// class CachedThreadPool {

// private:

//     std::thread repear;
//     std::vector<int> idle;
//     std::mutex reapmtx;
//     std::condition_variable reap;

//     int tidCounter;
//     std::unordered_map<int, std::unique_ptr<std::thread>> threads;
//     std::atomic<size_t> tnum;
//     std::chrono::seconds timeout;
    
//     std::queue<std::function<void()>> tasks;

//     std::mutex mtx;
//     std::condition_variable cond;

//     std::atomic<bool> running;

// public:

//     CachedThreadPool(std::chrono::seconds t) : tidCounter(1), timeout(t), running(true) {
//         // create a long live repear thread
//         repear = std::thread([this] () -> void {
//             while (true) {
//                 std::unique_lock<std::mutex> lock(reapmtx);
//                 reap.wait(lock, [this] () -> bool { return !idle.empty(); });
//                 int id = idle.back();
//                 idle.pop_back();
//                 if (threads[id]->joinable()) {
//                     threads[id]->join();
//                     threads.erase(id);
//                 }
//             }
//         });
//     }

//     ~CachedThreadPool() {}

//     template<typename Func, typename... Args>
//     auto Exec(Func&& fun, Args&&... args) -> std::future<std::invoke_result_t<Func, Args...>> {
//         if (!running) {
//             std::cerr << "Failed to execute: cached thread pool is no longer running." << std::endl;
//         }
//         using return_type = std::invoke_result_t<Func, Args...>;
//         auto task = std::make_shared<std::packaged_task<return_type()>>(
//             std::bind(std::forward<Func>(fun), std::forward<Args>(args)...)
//         );
//         {
//             std::unique_lock<std::mutex> lock(mtx);
//             tasks.emplace([task] () -> bool { (*task)(); });
//             cond.notify_one();
//         }
//         return task->get_future();
//     }

// private: // helpers

//     void threadfunc(int id) {
//         while (true) {
//             std::function<void()> task;
//             {
//                 std::unique_lock<std::mutex> lock(mtx);
//                 if (!cond.wait_for(lock, timeout, [this] () -> bool { return !tasks.empty(); })) {
//                     // the thread has been blocked over timeout seconds because of empty task queue, break the loop
//                     break;
//                 }
//                 task = tasks.front();
//                 tasks.pop();
//             }
//             std::cout << "CachedThread - " << id << " is executing task." << std::endl;
//             task();
//             std::cout << "CachedThread - " << id << " finished the task." << std::endl;
//         }
//     }

// };