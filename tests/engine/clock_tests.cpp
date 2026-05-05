#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>

#include <turdus/core/Bpm.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/engine/Clock.hpp>

using namespace std::chrono_literals;
using namespace turdus::core;
using namespace turdus::engine;

TEST_CASE("elapsed_to_ticks: 1 minute at 60 BPM yields 60 * PPQ ticks",
          "[engine][clock][math]") {
    auto t = elapsed_to_ticks(60'000'000us, Bpm{60.0});
    REQUIRE(t == Tick{60 * 960});
}

TEST_CASE("elapsed_to_ticks: 1 second at 120 BPM yields 2 * PPQ ticks",
          "[engine][clock][math]") {
    auto t = elapsed_to_ticks(1'000'000us, Bpm{120.0});
    REQUIRE(t == Tick{2 * 960});
}

TEST_CASE("elapsed_to_ticks: scales linearly with tempo", "[engine][clock][math]") {
    auto a = elapsed_to_ticks(1'000'000us, Bpm{60.0});
    auto b = elapsed_to_ticks(1'000'000us, Bpm{120.0});
    REQUIRE(b.value() == 2 * a.value());
}

TEST_CASE("elapsed_to_ticks: zero elapsed yields zero ticks", "[engine][clock][math]") {
    REQUIRE(elapsed_to_ticks(0us, Bpm{120.0}) == Tick{0});
}

TEST_CASE("Clock starts and stops cleanly", "[engine][clock]") {
    Clock clock{1ms};
    REQUIRE_FALSE(clock.is_running());
    clock.start();
    REQUIRE(clock.is_running());
    std::this_thread::sleep_for(5ms);
    clock.stop();
    REQUIRE_FALSE(clock.is_running());
}

TEST_CASE("Clock::start is idempotent", "[engine][clock]") {
    Clock clock{1ms};
    clock.start();
    clock.start();  // second call should be a no-op
    REQUIRE(clock.is_running());
    clock.stop();
}
