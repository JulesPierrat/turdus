#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include <turdus/core/Bpm.hpp>
#include <turdus/core/Tick.hpp>

namespace turdus::engine {

class Engine;

// Real-time clock that periodically calls Engine::advance with a tick delta computed
// from wall-time × tempo × PPQ. Phase 4 implementation: std::thread + sleep_until.
//
// Thread safety: start() and stop() are called from the UI/main thread; the internal
// thread is the only one calling Engine::advance. A fractional accumulator carries
// sub-tick remainders across slices to eliminate rounding drift over time.
class Clock {
public:
    using Duration = std::chrono::microseconds;

    explicit Clock(Duration slice = std::chrono::milliseconds(1));
    ~Clock();

    Clock(const Clock&) = delete;
    Clock& operator=(const Clock&) = delete;

    void attach(Engine* engine) noexcept { engine_ = engine; }

    void start();
    void stop();

    bool is_running() const noexcept { return running_.load(std::memory_order_acquire); }
    Duration slice() const noexcept { return slice_; }

private:
    Duration slice_;
    Engine* engine_{nullptr};
    std::atomic<bool> running_{false};
    std::thread thread_;

    void thread_loop();
};

// Free function for testing the time-to-tick math in isolation. Truncates toward
// zero — the Clock's internal accumulator handles the lost fractional part.
core::Tick elapsed_to_ticks(Clock::Duration elapsed, core::Bpm tempo) noexcept;

}  // namespace turdus::engine
