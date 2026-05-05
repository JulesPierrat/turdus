#include <catch2/catch_test_macros.hpp>

#include <turdus/core/Channel.hpp>
#include <turdus/model/Pattern.hpp>
#include <turdus/model/Track.hpp>

using namespace turdus::core;
using namespace turdus::model;

TEST_CASE("Track default-constructs", "[model][track]") {
    Track t;
    REQUIRE(t.name().empty());
    REQUIRE(t.port_label().empty());
    REQUIRE(t.channel() == Channel{0});
    REQUIRE_FALSE(t.muted());
    REQUIRE_FALSE(t.soloed());
    REQUIRE(t.transpose() == 0);
    REQUIRE(t.patterns().empty());
}

TEST_CASE("Track setters", "[model][track]") {
    Track t;
    t.set_name("Lead");
    t.set_port_label("Synth A");
    t.set_channel(Channel{2});
    t.set_muted(true);
    t.set_soloed(true);
    t.set_transpose(-12);

    REQUIRE(t.name() == "Lead");
    REQUIRE(t.port_label() == "Synth A");
    REQUIRE(t.channel() == Channel{2});
    REQUIRE(t.muted());
    REQUIRE(t.soloed());
    REQUIRE(t.transpose() == -12);
}

TEST_CASE("Track::set_transpose clamps", "[model][track]") {
    Track t;
    t.set_transpose(Track::kMinTranspose - 100);
    REQUIRE(t.transpose() == Track::kMinTranspose);
    t.set_transpose(Track::kMaxTranspose + 100);
    REQUIRE(t.transpose() == Track::kMaxTranspose);
}

TEST_CASE("Track::add_pattern returns an id usable to find/remove", "[model][track]") {
    Track t;
    auto id = t.add_pattern(Pattern{"P1", Tick{960}, Channel{0}});
    REQUIRE(id.is_valid());
    REQUIRE(t.patterns().size() == 1);

    auto* found = t.find_pattern(id);
    REQUIRE(found != nullptr);
    REQUIRE(found->name() == "P1");

    REQUIRE(t.remove_pattern(id));
    REQUIRE(t.patterns().empty());
    REQUIRE(t.find_pattern(id) == nullptr);
}

TEST_CASE("Track::remove_pattern returns false for unknown id", "[model][track]") {
    Track t;
    REQUIRE_FALSE(t.remove_pattern(PatternId::from_raw(123)));
}

TEST_CASE("Track equality is content-based and survives copy", "[model][track]") {
    Track a("Bass", "Synth", Channel{1});
    Track b = a;
    REQUIRE(a == b);

    a.set_muted(true);
    REQUIRE(a != b);
    REQUIRE_FALSE(b.muted());
}
