#include <catch2/catch_test_macros.hpp>

#include <turdus/core/Bpm.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/engine/Transport.hpp>

using namespace turdus::core;
using namespace turdus::engine;

TEST_CASE("Transport defaults to stopped at tick 0, default tempo", "[engine][transport]") {
    Transport t;
    REQUIRE(t.state() == Transport::State::Stopped);
    REQUIRE(t.position() == Tick{0});
    REQUIRE(t.tempo() == Bpm{Bpm::kDefault});
    REQUIRE_FALSE(t.is_playing());
}

TEST_CASE("Transport play / pause / stop", "[engine][transport]") {
    Transport t;
    t.play();
    REQUIRE(t.is_playing());

    t.advance(Tick{500});
    REQUIRE(t.position() == Tick{500});

    t.pause();
    REQUIRE_FALSE(t.is_playing());
    REQUIRE(t.position() == Tick{500});  // pause keeps position

    t.stop();
    REQUIRE_FALSE(t.is_playing());
    REQUIRE(t.position() == Tick{0});  // stop resets position
}

TEST_CASE("Transport seek and set_tempo", "[engine][transport]") {
    Transport t;
    t.seek(Tick{1234});
    REQUIRE(t.position() == Tick{1234});

    t.set_tempo(Bpm{140.0});
    REQUIRE(t.tempo() == Bpm{140.0});
}

TEST_CASE("Transport snapshot reads all fields together", "[engine][transport]") {
    Transport t;
    t.play();
    t.seek(Tick{960});
    t.set_tempo(Bpm{132.0});

    auto s = t.snapshot();
    REQUIRE(s.state == Transport::State::Playing);
    REQUIRE(s.position == Tick{960});
    REQUIRE(s.tempo == Bpm{132.0});
}
