#include <turdus/engine/Clock.hpp>

#include <turdus/core/Ppq.hpp>
#include <turdus/engine/Engine.hpp>

namespace turdus::engine {

namespace {
constexpr double kMicrosPerMinute = 60'000'000.0;

double elapsed_to_ticks_double(Clock::Duration elapsed, core::Bpm tempo) noexcept {
    return static_cast<double>(elapsed.count()) * tempo.value()
           * static_cast<double>(core::kPpq) / kMicrosPerMinute;
}
}  // namespace

core::Tick elapsed_to_ticks(Clock::Duration elapsed, core::Bpm tempo) noexcept {
    return core::Tick{static_cast<core::Tick::value_type>(elapsed_to_ticks_double(elapsed, tempo))};
}

Clock::Clock(Duration slice) : slice_(slice) {}

Clock::~Clock() { stop(); }

void Clock::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return;  // already running
    }
    thread_ = std::thread(&Clock::thread_loop, this);
}

void Clock::stop() {
    running_.store(false, std::memory_order_release);
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Clock::thread_loop() {
    using namespace std::chrono;
    auto last = steady_clock::now();
    auto next = last + slice_;

    double accumulator = 0.0;

    while (running_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_until(next);
        const auto now = steady_clock::now();
        const auto elapsed = duration_cast<Duration>(now - last);
        last = now;
        next = now + slice_;

        if (engine_ == nullptr) {
            continue;
        }

        const auto tempo = engine_->transport().tempo();
        accumulator += elapsed_to_ticks_double(elapsed, tempo);
        const auto whole = static_cast<core::Tick::value_type>(accumulator);
        if (whole > 0) {
            accumulator -= static_cast<double>(whole);
            engine_->advance(core::Tick{whole});
        }
    }
}

}  // namespace turdus::engine
