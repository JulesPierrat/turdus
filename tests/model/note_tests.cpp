#include <catch2/catch_test_macros.hpp>

#include <turdus/core/Pitch.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/core/Velocity.hpp>
#include <turdus/model/Note.hpp>

using namespace turdus::core;
using namespace turdus::model;

TEST_CASE("Note end() is start + length", "[model][note]") {
    Note n{Pitch{60}, Tick{100}, Tick{240}, Velocity{90}};
    REQUIRE(n.end() == Tick{340});
}

TEST_CASE("Note equality is field-wise", "[model][note]") {
    Note a{Pitch{60}, Tick{0}, Tick{240}, Velocity{100}};
    Note b{Pitch{60}, Tick{0}, Tick{240}, Velocity{100}};
    REQUIRE(a == b);

    Note c{Pitch{61}, Tick{0}, Tick{240}, Velocity{100}};
    REQUIRE(a != c);
}
