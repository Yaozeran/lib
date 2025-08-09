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

#include <chrono>


template<size_t max, size_t core>
class ThreadPool {

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

    ThreadPool() : running(true) {
        for (size_t i = 0; i < core; ++i) {
            std::unique_ptr<std::thread> threadptr = std::make_unique<std::thread>(&ThreadPool::fixed, this);
            threads.emplace(threadptr->get_id(), std::move(threadptr));
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            running = false;
        }
        for (auto& [id, t]: threads) {
            if (t->joinable()) {
                t->join();
                std::cout << "Thread - " << id << " joins the main thread." << std::endl;
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
        if (!running) {
            std::cerr << "Failed to execute: thread pool is no longer running" << std::endl;
        }
        using return_type = std::invoke_result_t<Func, Args...>;
        // package task
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<Func>(fun), std::forward<Args>(args)...)
        );
        // enqueue
        {
            std::unique_lock<std::mutex> lock(mtx);
            tasks.emplace([task] () -> bool { (*task)(); }); // captures task by value, holds a copy of the shared_ptr
            cond.notify_one();
        }
        // return result
        return task->get_future();
    }

private:

    void fixed() {
        while (true) {
            std::function<void()> task;
            // take task out of task queue
            {
                std::unique_lock<std::mutex> lock(mtx);
                cond.wait(lock, [this] () -> bool { return !tasks.empty(); });
                task = tasks.front();
                tasks.pop();
            }
            // exec the task
            task();
        }
    }

    void cached() {

    }

};