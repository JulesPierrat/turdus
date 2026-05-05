#include <catch2/catch_test_macros.hpp>

#include <turdus/core/Bpm.hpp>
#include <turdus/core/Channel.hpp>
#include <turdus/model/Project.hpp>
#include <turdus/model/TimeSignature.hpp>
#include <turdus/model/Track.hpp>

using namespace turdus::core;
using namespace turdus::model;

TEST_CASE("Project default state", "[model][project]") {
    Project p;
    REQUIRE(p.name().empty());
    REQUIRE(p.tempo() == Bpm{Bpm::kDefault});
    REQUIRE(p.time_signature() == TimeSignature{});
    REQUIRE(p.tracks().empty());
    REQUIRE(p.arrangement().empty());
    REQUIRE(p.port_mappings().empty());
}

TEST_CASE("Project tempo and time signature", "[model][project]") {
    Project p;
    p.set_tempo(Bpm{140.0});
    p.set_time_signature(TimeSignature{6, 8});
    REQUIRE(p.tempo() == Bpm{140.0});
    REQUIRE(p.time_signature() == TimeSignature{6, 8});
}

TEST_CASE("Project::add_track / find_track / remove_track", "[model][project]") {
    Project p;
    auto t1 = p.add_track(Track{"Drums", "Drum machine", Channel{9}});
    auto t2 = p.add_track(Track{"Bass", "Synth A", Channel{1}});
    REQUIRE(p.tracks().size() == 2);

    REQUIRE(p.find_track(t1) != nullptr);
    REQUIRE(p.find_track(t1)->name() == "Drums");
    REQUIRE(p.find_track(t2)->name() == "Bass");

    REQUIRE(p.remove_track(t1));
    REQUIRE(p.tracks().size() == 1);
    REQUIRE(p.find_track(t1) == nullptr);
    REQUIRE_FALSE(p.remove_track(t1));
}

TEST_CASE("Removing a track removes its placements from arrangement", "[model][project]") {
    Project p;
    auto t1 = p.add_track(Track{});
    auto t2 = p.add_track(Track{});
    p.add_placement(PatternPlacement{t1, PatternId::from_raw(1), Tick{0}});
    p.add_placement(PatternPlacement{t2, PatternId::from_raw(2), Tick{960}});
    p.add_placement(PatternPlacement{t1, PatternId::from_raw(3), Tick{1920}});

    REQUIRE(p.arrangement().size() == 3);
    REQUIRE(p.remove_track(t1));
    REQUIRE(p.arrangement().size() == 1);
    REQUIRE(p.arrangement().front().track_id == t2);
}

TEST_CASE("Project arrangement add/remove/clear", "[model][project]") {
    Project p;
    auto t = p.add_track(Track{});
    p.add_placement(PatternPlacement{t, PatternId::from_raw(1), Tick{0}});
    p.add_placement(PatternPlacement{t, PatternId::from_raw(2), Tick{960}});
    REQUIRE(p.arrangement().size() == 2);

    REQUIRE(p.remove_placement(0));
    REQUIRE(p.arrangement().size() == 1);
    REQUIRE(p.arrangement().front().pattern_id == PatternId::from_raw(2));

    REQUIRE_FALSE(p.remove_placement(99));
    p.clear_arrangement();
    REQUIRE(p.arrangement().empty());
}

TEST_CASE("Project port mappings: unique by label", "[model][project]") {
    Project p;
    p.add_port_mapping(MidiPortMapping{"Synth", "IAC Bus 1", false});
    p.add_port_mapping(MidiPortMapping{"Drums", "IAC Bus 2", true});
    REQUIRE(p.port_mappings().size() == 2);

    // Duplicate labels are ignored.
    p.add_port_mapping(MidiPortMapping{"Synth", "DIFFERENT", true});
    REQUIRE(p.port_mappings().size() == 2);
    REQUIRE(p.find_port_mapping("Synth")->device_name == "IAC Bus 1");

    REQUIRE(p.find_port_mapping("Drums")->send_clock);
    REQUIRE(p.find_port_mapping("missing") == nullptr);

    REQUIRE(p.remove_port_mapping("Synth"));
    REQUIRE(p.port_mappings().size() == 1);
    REQUIRE_FALSE(p.remove_port_mapping("Synth"));
}

TEST_CASE("Project is value-copyable (snapshot semantics)", "[model][project]") {
    Project original;
    original.set_name("song");
    original.set_tempo(Bpm{132.0});
    auto track_id = original.add_track(Track{"Lead", "Synth", Channel{0}});

    Project snapshot = original;
    REQUIRE(snapshot == original);

    original.set_tempo(Bpm{150.0});
    original.remove_track(track_id);

    REQUIRE(snapshot.tempo() == Bpm{132.0});
    REQUIRE(snapshot.tracks().size() == 1);
    REQUIRE(snapshot != original);
}
