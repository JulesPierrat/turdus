#include <catch2/catch_test_macros.hpp>

#include <turdus/core/Channel.hpp>
#include <turdus/core/Pitch.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/core/Velocity.hpp>
#include <turdus/engine/Engine.hpp>
#include <turdus/midi/FakeMidiPort.hpp>
#include <turdus/midi/MidiMessage.hpp>
#include <turdus/model/Note.hpp>
#include <turdus/model/Pattern.hpp>

using namespace turdus::core;
using namespace turdus::engine;
using namespace turdus::midi;
using namespace turdus::model;

namespace {
Note make_note(int pitch, int start, int length, int velocity = 100) {
    return Note{Pitch{pitch}, Tick{start}, Tick{length}, Velocity{velocity}};
}

Pattern make_pattern(int length, std::initializer_list<Note> notes) {
    Pattern p{"test", Tick{length}, Channel{0}};
    for (auto n : notes) {
        p.add_note(n);
    }
    return p;
}
}  // namespace

TEST_CASE("Engine emits nothing when stopped", "[engine][playback]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.set_pattern(make_pattern(960, {make_note(60, 0, 240)}), Channel{0});
    // not playing
    eng.advance(Tick{2000});
    REQUIRE(port.sent().empty());
}

TEST_CASE("Engine emits nothing when no pattern is set", "[engine][playback]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.transport().play();
    eng.advance(Tick{2000});
    REQUIRE(port.sent().empty());
}

TEST_CASE("Engine emits note_on at start, note_off at start+length", "[engine][playback]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.set_pattern(make_pattern(960, {make_note(60, 0, 240, 100)}), Channel{0});
    eng.transport().play();

    // Range [0, 100): note_on at 0; note_off scheduled for 240 (still pending).
    eng.advance(Tick{100});
    REQUIRE(port.sent().size() == 1);
    REQUIRE(port.sent()[0].msg == MidiMessage::note_on(Channel{0}, Pitch{60}, Velocity{100}));
    REQUIRE(port.sent()[0].deadline == Tick{0});

    // Range [100, 300): note_off fires at 240.
    eng.advance(Tick{200});
    REQUIRE(port.sent().size() == 2);
    REQUIRE(port.sent()[1].msg == MidiMessage::note_off(Channel{0}, Pitch{60}));
    REQUIRE(port.sent()[1].deadline == Tick{240});
}

TEST_CASE("Engine fires note_off in the same advance call when length fits in slice",
          "[engine][playback]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.set_pattern(make_pattern(960, {make_note(60, 0, 100)}), Channel{0});
    eng.transport().play();

    eng.advance(Tick{500});
    REQUIRE(port.sent().size() == 2);
    REQUIRE(port.sent()[0].msg.status == 0x90);
    REQUIRE(port.sent()[1].msg.status == 0x80);
    REQUIRE(port.sent()[0].deadline == Tick{0});
    REQUIRE(port.sent()[1].deadline == Tick{100});
}

TEST_CASE("Engine loops the pattern", "[engine][playback]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    // 1000-tick pattern with one note: on at t=0, off at t=200.
    eng.set_pattern(make_pattern(1000, {make_note(60, 0, 200)}), Channel{0});
    eng.transport().play();

    // Advance 2500 ticks: 3 iterations partially covered.
    eng.advance(Tick{2500});

    REQUIRE(port.sent().size() == 6);  // 3 on + 3 off
    REQUIRE(port.sent()[0].deadline == Tick{0});
    REQUIRE(port.sent()[0].msg.status == 0x90);
    REQUIRE(port.sent()[1].deadline == Tick{200});
    REQUIRE(port.sent()[1].msg.status == 0x80);
    REQUIRE(port.sent()[2].deadline == Tick{1000});
    REQUIRE(port.sent()[3].deadline == Tick{1200});
    REQUIRE(port.sent()[4].deadline == Tick{2000});
    REQUIRE(port.sent()[5].deadline == Tick{2200});
}

TEST_CASE("Engine handles a note crossing the loop boundary", "[engine][playback]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    // Note: start=900, length=300 → off at 1200, beyond the 1000-tick pattern length.
    eng.set_pattern(make_pattern(1000, {make_note(60, 900, 300)}), Channel{0});
    eng.transport().play();

    eng.advance(Tick{2000});

    // First iter: on at 900, off at 1200. Second iter: on at 1900, off at 2200 (pending).
    REQUIRE(port.sent().size() == 3);
    REQUIRE(port.sent()[0].deadline == Tick{900});
    REQUIRE(port.sent()[0].msg.status == 0x90);
    REQUIRE(port.sent()[1].deadline == Tick{1200});
    REQUIRE(port.sent()[1].msg.status == 0x80);
    REQUIRE(port.sent()[2].deadline == Tick{1900});
    REQUIRE(port.sent()[2].msg.status == 0x90);
}

TEST_CASE("Engine::all_notes_off releases held notes", "[engine][playback]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    Pattern p{"test", Tick{960}, Channel{0}};
    p.add_note(make_note(60, 0, 800));
    p.add_note(make_note(64, 100, 800));
    eng.set_pattern(p, Channel{0});
    eng.transport().play();

    eng.advance(Tick{200});
    REQUIRE(port.sent().size() == 2);  // both note_ons, offs still pending

    eng.all_notes_off();
    REQUIRE(port.sent().size() == 4);
    REQUIRE(port.sent()[2].msg.status == 0x80);
    REQUIRE(port.sent()[3].msg.status == 0x80);
}

TEST_CASE("Engine emits multiple notes in start order", "[engine][playback]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    Pattern p{"test", Tick{960}, Channel{0}};
    p.add_note(make_note(67, 480, 100));
    p.add_note(make_note(60, 0, 100));
    p.add_note(make_note(64, 240, 100));
    eng.set_pattern(p, Channel{0});
    eng.transport().play();

    eng.advance(Tick{600});

    // 6 events: on/off for each of the three notes, in chronological order.
    REQUIRE(port.sent().size() == 6);
    REQUIRE(port.sent()[0].msg.data1 == 60);  // C on
    REQUIRE(port.sent()[1].msg.data1 == 60);  // C off
    REQUIRE(port.sent()[2].msg.data1 == 64);  // E on
    REQUIRE(port.sent()[3].msg.data1 == 64);  // E off
    REQUIRE(port.sent()[4].msg.data1 == 67);  // G on
    REQUIRE(port.sent()[5].msg.data1 == 67);  // G off
}
