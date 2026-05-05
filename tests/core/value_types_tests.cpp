#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <turdus/core/Beats.hpp>
#include <turdus/core/Bpm.hpp>
#include <turdus/core/Channel.hpp>
#include <turdus/core/Pitch.hpp>
#include <turdus/core/Ppq.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/core/Velocity.hpp>

using namespace turdus::core;

TEST_CASE("Tick", "[core][tick]") {
    SECTION("default-constructs to zero") {
        REQUIRE(Tick{}.value() == 0);
    }
    SECTION("explicit construction and value()") {
        REQUIRE(Tick{960}.value() == 960);
        REQUIRE(Tick{-12}.value() == -12);
    }
    SECTION("addition / subtraction") {
        REQUIRE((Tick{100} + Tick{50}) == Tick{150});
        REQUIRE((Tick{100} - Tick{50}) == Tick{50});
        Tick t{10};
        t += Tick{5};
        REQUIRE(t == Tick{15});
        t -= Tick{20};
        REQUIRE(t == Tick{-5});
    }
    SECTION("ordering") {
        REQUIRE(Tick{0} < Tick{1});
        REQUIRE(Tick{1} > Tick{0});
        REQUIRE(Tick{42} == Tick{42});
    }
}

TEST_CASE("Beats", "[core][beats]") {
    SECTION("default-constructs to zero") {
        REQUIRE(Beats{}.value() == 0.0);
    }
    SECTION("addition / subtraction") {
        REQUIRE((Beats{1.5} + Beats{0.5}).value() == 2.0);
        REQUIRE((Beats{1.0} - Beats{0.25}).value() == 0.75);
    }
    SECTION("ordering") {
        REQUIRE(Beats{1.0} < Beats{2.0});
    }
}

TEST_CASE("Bpm", "[core][bpm]") {
    SECTION("default tempo is 120") {
        REQUIRE(Bpm{}.value() == Bpm::kDefault);
    }
    SECTION("clamps below kMin") {
        REQUIRE(Bpm{-10.0}.value() == Bpm::kMin);
        REQUIRE(Bpm{0.0}.value() == Bpm::kMin);
        REQUIRE(Bpm{Bpm::kMin - 1.0}.value() == Bpm::kMin);
    }
    SECTION("clamps above kMax") {
        REQUIRE(Bpm{10000.0}.value() == Bpm::kMax);
        REQUIRE(Bpm{Bpm::kMax + 1.0}.value() == Bpm::kMax);
    }
    SECTION("preserves in-range values") {
        REQUIRE(Bpm{120.0}.value() == 120.0);
        REQUIRE(Bpm{60.5}.value() == 60.5);
    }
}

TEST_CASE("Pitch", "[core][pitch]") {
    SECTION("default is 0") {
        REQUIRE(Pitch{}.value() == 0);
    }
    SECTION("preserves in-range values") {
        REQUIRE(Pitch{60}.value() == 60);
        REQUIRE(Pitch{0}.value() == 0);
        REQUIRE(Pitch{127}.value() == 127);
    }
    SECTION("clamps to [0, 127]") {
        REQUIRE(Pitch{-1}.value() == 0);
        REQUIRE(Pitch{128}.value() == 127);
        REQUIRE(Pitch{500}.value() == 127);
    }
}

TEST_CASE("Velocity", "[core][velocity]") {
    SECTION("default is 100") {
        REQUIRE(Velocity{}.value() == Velocity::kDefault);
    }
    SECTION("clamps to [0, 127]") {
        REQUIRE(Velocity{-5}.value() == 0);
        REQUIRE(Velocity{200}.value() == 127);
        REQUIRE(Velocity{64}.value() == 64);
    }
}

TEST_CASE("Channel", "[core][channel]") {
    SECTION("default is 0 (display 1)") {
        REQUIRE(Channel{}.value() == 0);
        REQUIRE(Channel{}.display() == 1);
    }
    SECTION("clamps to [0, 15]") {
        REQUIRE(Channel{-3}.value() == 0);
        REQUIRE(Channel{20}.value() == 15);
        REQUIRE(Channel{7}.value() == 7);
    }
    SECTION("display is 1-indexed") {
        REQUIRE(Channel{0}.display() == 1);
        REQUIRE(Channel{15}.display() == 16);
    }
}

TEST_CASE("Ppq conversions", "[core][ppq]") {
    SECTION("kPpq is 960") {
        REQUIRE(kPpq == 960);
    }
    SECTION("beats_to_ticks of 1 beat is one quarter note") {
        REQUIRE(beats_to_ticks(Beats{1.0}) == Tick{960});
        REQUIRE(beats_to_ticks(Beats{4.0}) == Tick{3840});
        REQUIRE(beats_to_ticks(Beats{0.5}) == Tick{480});
    }
    SECTION("ticks_to_beats inverse on integer-aligned values") {
        REQUIRE(ticks_to_beats(Tick{960}).value() == 1.0);
        REQUIRE(ticks_to_beats(Tick{0}).value() == 0.0);
        REQUIRE(ticks_to_beats(Tick{480}).value() == 0.5);
    }
    SECTION("round-trip is exact starting from any integer tick") {
        for (auto t : {Tick{0}, Tick{1}, Tick{960}, Tick{12345}, Tick{-7}}) {
            REQUIRE(beats_to_ticks(ticks_to_beats(t)) == t);
        }
    }
    SECTION("beats_to_ticks rounds to nearest tick") {
        // 1/PPQ beat = 1 tick exactly
        REQUIRE(beats_to_ticks(Beats{1.0 / 960.0}) == Tick{1});
        // 0.5/PPQ beat rounds up to 1 tick
        REQUIRE(beats_to_ticks(Beats{0.5 / 960.0}) == Tick{1});
    }
}
