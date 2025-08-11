#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

// Simple Time Wheel implementation in C++17
// Features:
// - Fixed number of slots (wheel_size), each slot covers tick_ms milliseconds
// - Schedule timers with arbitrary delays (>= tick_ms)
// - start() runs an internal thread that advances the wheel every tick_ms
// - cancel timer by id
// - thread-safe
//
// Usage:
//   TimeWheel tw(100 /*tick ms*/, 512 /*slots*/);
//   tw.start();
//   auto id = tw.addTimer(1500, []{ std::cout<<"timeout\n"; });
//   tw.cancelTimer(id);
//   tw.stop();

class TimeWheel {

    using Clock = std::chrono::steady_clock;
    using TimerId = uint64_t;
    using Callback = std::function<void()>;

private:

    struct TimerEntry {
        TimerId id{0};
        uint64_t rotation{0};
        Callback cb;
    };

    struct IdMeta { size_t slot; uint64_t rotation; };

    uint32_t tick_ms_;
    size_t wheel_size_;

    std::vector<std::vector<TimerEntry>> slots_;
    size_t current_slot_;

    std::mutex mutex_;
    std::unordered_map<TimerId, IdMeta> id_map_;

    std::atomic<bool> running_;
    std::thread worker_;

    std::atomic<TimerId> next_id_;

public:


    TimeWheel(uint32_t tick_ms = 100, size_t wheel_size = 512)
        : tick_ms_(tick_ms),
          wheel_size_(wheel_size),
          slots_(wheel_size),
          current_slot_(0),
          running_(false),
          next_id_(1) {}

    ~TimeWheel() {
        stop();
    }

    // Start the internal ticking thread. If already started, does nothing.
    void start() {
        bool expected = false;
        if (!running_.compare_exchange_strong(expected, true)) return;
        worker_ = std::thread([this]() { this->run(); });
    }

    // Stop and join the internal thread. Will execute no further callbacks.
    void stop() {
        bool expected = true;
        if (!running_.compare_exchange_strong(expected, false)) return;
        if (worker_.joinable()) worker_.join();
    }

    // Schedule a timer that fires after delay_ms milliseconds.
    // Returns a TimerId which can be used to cancel.
    TimerId addTimer(uint64_t delay_ms, Callback cb) {
        if (!cb) return 0;
        if (delay_ms < tick_ms_) delay_ms = tick_ms_; // floor to tick

        // compute ticks and rotations
        uint64_t ticks = (delay_ms + tick_ms_ - 1) / tick_ms_; // ceil
        uint64_t rotation = ticks / wheel_size_;
        size_t slot = (current_slot_ + (ticks % wheel_size_)) % wheel_size_;

        TimerEntry entry;
        entry.id = next_id_.fetch_add(1);
        entry.rotation = rotation;
        entry.cb = std::move(cb);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            slots_[slot].push_back(entry);
            id_map_[entry.id] = {slot, entry.rotation};
        }

        return entry.id;
    }

    // Cancel a timer by id. Returns true if removed, false if not found or already fired.
    bool cancelTimer(TimerId id) {
        if (id == 0) return false;
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = id_map_.find(id);
        if (it == id_map_.end()) return false;
        size_t slot = it->second.slot;
        uint64_t rotation = it->second.rotation;
        auto &vec = slots_[slot];
        for (auto vit = vec.begin(); vit != vec.end(); ++vit) {
            if (vit->id == id && vit->rotation == rotation) {
                vec.erase(vit);
                id_map_.erase(it);
                return true;
            }
        }
        return false;
    }

private:

    void run() {
        auto next_tick = Clock::now();
        while (running_) {
            next_tick += std::chrono::milliseconds(tick_ms_);
            std::this_thread::sleep_until(next_tick);
            tick();
        }
    }

    void tick() {
        std::vector<Callback> to_run;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            // move out the slot list to process without holding callbacks
            auto &bucket = slots_[current_slot_];
            if (!bucket.empty()) {
                // iterate and collect those with rotation==0
                auto it = bucket.begin();
                while (it != bucket.end()) {
                    if (it->rotation == 0) {
                        to_run.push_back(it->cb);
                        id_map_.erase(it->id);
                        it = bucket.erase(it);
                    } else {
                        // decrement rotation and keep
                        it->rotation -= 1;
                        ++it;
                    }
                }
            }
            // advance slot
            current_slot_ = (current_slot_ + 1) % wheel_size_;
        }

        // run callbacks without holding the lock
        for (auto &f : to_run) {
            f();
        }
    }
};

// Example usage
#ifdef TIMEWHEEL_TEST
int main() {
    TimeWheel tw(100, 128); // 100 ms tick, 128 slots -> max approx 12.8s without rotations
    tw.start();

    std::cout << "Scheduling timers...\n";
    tw.addTimer(500, []{ std::cout << "500ms fired\n"; });
    tw.addTimer(1500, []{ std::cout << "1500ms fired\n"; });
    tw.addTimer(3500, []{ std::cout << "3500ms fired\n"; });

    // schedule a repeating timer using addTimer from inside the callback (simple way)
    tw.addTimer(700, [&tw]() {
        static int n = 0;
        std::cout << "700ms fired " << ++n << "\n";
        if (n < 5) tw.addTimer(700, [&tw](){});
    });

    std::this_thread::sleep_for(std::chrono::seconds(5));
    tw.stop();
    std::cout << "Stopped\n";
}
#endif
