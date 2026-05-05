#include <catch2/catch_test_macros.hpp>

#include <turdus/core/Channel.hpp>
#include <turdus/core/Pitch.hpp>
#include <turdus/core/Velocity.hpp>
#include <turdus/midi/MidiMessage.hpp>

using namespace turdus::core;
using namespace turdus::midi;

TEST_CASE("note_on encodes status, pitch, velocity", "[midi][message]") {
    auto m = MidiMessage::note_on(Channel{0}, Pitch{60}, Velocity{100});
    REQUIRE(m.status == 0x90);
    REQUIRE(m.data1 == 60);
    REQUIRE(m.data2 == 100);
    REQUIRE(m.length == 3);
}

TEST_CASE("note_on packs the channel into the low nibble of status", "[midi][message]") {
    REQUIRE(MidiMessage::note_on(Channel{0}, Pitch{60}, Velocity{1}).status == 0x90);
    REQUIRE(MidiMessage::note_on(Channel{1}, Pitch{60}, Velocity{1}).status == 0x91);
    REQUIRE(MidiMessage::note_on(Channel{15}, Pitch{60}, Velocity{1}).status == 0x9F);
}

TEST_CASE("note_off uses 0x80 with default release velocity 0", "[midi][message]") {
    auto m = MidiMessage::note_off(Channel{0}, Pitch{60});
    REQUIRE(m.status == 0x80);
    REQUIRE(m.data1 == 60);
    REQUIRE(m.data2 == 0);
    REQUIRE(m.length == 3);
}

TEST_CASE("note_off accepts a non-zero release velocity", "[midi][message]") {
    auto m = MidiMessage::note_off(Channel{2}, Pitch{72}, Velocity{42});
    REQUIRE(m.status == 0x82);
    REQUIRE(m.data2 == 42);
}

TEST_CASE("CC encodes controller and value, masking to 7 bits", "[midi][message]") {
    auto m = MidiMessage::cc(Channel{1}, 7, 100);
    REQUIRE(m.status == 0xB1);
    REQUIRE(m.data1 == 7);
    REQUIRE(m.data2 == 100);
    REQUIRE(m.length == 3);

    // Out-of-range values are masked to the low 7 bits (instead of UB-ing).
    auto masked = MidiMessage::cc(Channel{0}, 0xFF, 0xFF);
    REQUIRE(masked.data1 == 0x7F);
    REQUIRE(masked.data2 == 0x7F);
}

TEST_CASE("program_change is a 2-byte message", "[midi][message]") {
    auto m = MidiMessage::program_change(Channel{2}, 42);
    REQUIRE(m.status == 0xC2);
    REQUIRE(m.data1 == 42);
    REQUIRE(m.data2 == 0);
    REQUIRE(m.length == 2);
}

TEST_CASE("Real-time messages are single-byte with the right status", "[midi][message]") {
    REQUIRE(MidiMessage::clock().status == 0xF8);
    REQUIRE(MidiMessage::clock().length == 1);

    REQUIRE(MidiMessage::start().status == 0xFA);
    REQUIRE(MidiMessage::cont().status == 0xFB);
    REQUIRE(MidiMessage::stop().status == 0xFC);

    for (auto m : {MidiMessage::clock(), MidiMessage::start(), MidiMessage::cont(),
                   MidiMessage::stop()}) {
        REQUIRE(m.length == 1);
        REQUIRE(m.data1 == 0);
        REQUIRE(m.data2 == 0);
    }
}

TEST_CASE("MidiMessage equality is bytewise", "[midi][message]") {
    auto a = MidiMessage::note_on(Channel{0}, Pitch{60}, Velocity{100});
    auto b = MidiMessage::note_on(Channel{0}, Pitch{60}, Velocity{100});
    auto c = MidiMessage::note_on(Channel{0}, Pitch{61}, Velocity{100});
    REQUIRE(a == b);
    REQUIRE(a != c);
}
