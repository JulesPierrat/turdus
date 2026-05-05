#include <catch2/catch_test_macros.hpp>

#include <turdus/io/Serialization.hpp>

using namespace turdus::core;
using namespace turdus::model;

TEST_CASE("Note round-trips through JSON", "[io][serialization]") {
    Note original{Pitch{60}, Tick{100}, Tick{240}, Velocity{90}};
    nlohmann::json j = original;
    auto restored = j.get<Note>();
    REQUIRE(original == restored);
}

TEST_CASE("TimeSignature round-trips", "[io][serialization]") {
    TimeSignature original{6, 8};
    nlohmann::json j = original;
    auto restored = j.get<TimeSignature>();
    REQUIRE(original == restored);
}

TEST_CASE("Strong-typed core values round-trip", "[io][serialization]") {
    SECTION("Tick") {
        Tick original{12345};
        REQUIRE(nlohmann::json(original).get<Tick>() == original);
    }
    SECTION("Bpm") {
        Bpm original{132.5};
        REQUIRE(nlohmann::json(original).get<Bpm>() == original);
    }
    SECTION("Channel") {
        Channel original{7};
        REQUIRE(nlohmann::json(original).get<Channel>() == original);
    }
    SECTION("Pitch") {
        Pitch original{72};
        REQUIRE(nlohmann::json(original).get<Pitch>() == original);
    }
    SECTION("Velocity") {
        Velocity original{96};
        REQUIRE(nlohmann::json(original).get<Velocity>() == original);
    }
}

TEST_CASE("Pattern round-trips, preserving note ids and order", "[io][serialization]") {
    Pattern original{"Verse", Tick{1920}, Channel{2}};
    auto id1 = original.add_note(Note{Pitch{60}, Tick{0}, Tick{240}, Velocity{100}});
    auto id2 = original.add_note(Note{Pitch{64}, Tick{240}, Tick{240}, Velocity{90}});

    nlohmann::json j = original;
    auto restored = j.get<Pattern>();

    REQUIRE(original == restored);
    REQUIRE(restored.find_note(id1).has_value());
    REQUIRE(restored.find_note(id2).has_value());
}

TEST_CASE("Track round-trips, preserving pattern ids", "[io][serialization]") {
    Track original{"Lead", "Synth", Channel{1}};
    original.set_muted(true);
    original.set_transpose(-12);
    auto pid = original.add_pattern(Pattern{"P", Tick{960}, Channel{1}});

    nlohmann::json j = original;
    auto restored = j.get<Track>();

    REQUIRE(original == restored);
    REQUIRE(restored.find_pattern(pid) != nullptr);
}

TEST_CASE("Project round-trips with arrangement and port mappings", "[io][serialization]") {
    Project original;
    original.set_name("song");
    original.set_tempo(Bpm{132.0});
    original.set_time_signature(TimeSignature{6, 8});

    auto t1 = original.add_track(Track{"Lead", "Synth", Channel{0}});
    auto t2 = original.add_track(Track{"Drums", "Drum", Channel{9}});

    original.add_placement(PatternPlacement{t1, PatternId::from_raw(1), Tick{0}});
    original.add_placement(PatternPlacement{t2, PatternId::from_raw(2), Tick{960}});

    original.add_port_mapping(MidiPortMapping{"Synth", "Bus 1", false});
    original.add_port_mapping(MidiPortMapping{"Drum", "Bus 2", true});

    nlohmann::json j = original;
    auto restored = j.get<Project>();

    REQUIRE(original == restored);
}
