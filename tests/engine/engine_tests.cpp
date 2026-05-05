#include <catch2/catch_test_macros.hpp>

#include <initializer_list>

#include <turdus/core/Channel.hpp>
#include <turdus/core/Pitch.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/core/Velocity.hpp>
#include <turdus/engine/Engine.hpp>
#include <turdus/midi/FakeMidiPort.hpp>
#include <turdus/midi/MidiMessage.hpp>
#include <turdus/model/Note.hpp>
#include <turdus/model/Pattern.hpp>
#include <turdus/model/Project.hpp>
#include <turdus/model/Track.hpp>

using namespace turdus::core;
using namespace turdus::engine;
using namespace turdus::midi;
using namespace turdus::model;

namespace {
Note make_note(int pitch, int start, int length, int velocity = 100) {
    return Note{Pitch{pitch}, Tick{start}, Tick{length}, Velocity{velocity}};
}

// Build a single-track / single-pattern / single-placement project. Optionally
// configure a loop region matching the pattern length so the engine loops.
Project make_one_pattern_project(int pattern_length,
                                 std::initializer_list<Note> notes,
                                 bool loop = false,
                                 Channel channel = Channel{0}) {
    Project p;
    auto tid = p.add_track(Track{"t", "", channel});
    auto* track = p.find_track(tid);
    Pattern pat{"p", Tick{pattern_length}, channel};
    for (auto n : notes) {
        pat.add_note(n);
    }
    auto pid = track->add_pattern(std::move(pat));
    p.add_placement(PatternPlacement{tid, pid, Tick{0}});
    if (loop) {
        p.set_loop(LoopRegion{Tick{0}, Tick{pattern_length}});
    }
    return p;
}
}  // namespace

TEST_CASE("Engine emits nothing when stopped", "[engine][playback]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.set_project(make_one_pattern_project(960, {make_note(60, 0, 240)}));
    // not playing
    eng.advance(Tick{2000});
    REQUIRE(port.sent().empty());
}

TEST_CASE("Engine emits nothing when no project is set", "[engine][playback]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.transport().play();
    eng.advance(Tick{2000});
    REQUIRE(port.sent().empty());
}

TEST_CASE("Engine emits note_on at start, note_off at start+length",
          "[engine][playback]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.set_project(make_one_pattern_project(960, {make_note(60, 0, 240, 100)}));
    eng.transport().play();

    eng.advance(Tick{100});
    REQUIRE(port.sent().size() == 1);
    REQUIRE(port.sent()[0].msg == MidiMessage::note_on(Channel{0}, Pitch{60}, Velocity{100}));
    REQUIRE(port.sent()[0].deadline == Tick{0});

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
    eng.set_project(make_one_pattern_project(960, {make_note(60, 0, 100)}));
    eng.transport().play();

    eng.advance(Tick{500});
    REQUIRE(port.sent().size() == 2);
    REQUIRE(port.sent()[0].msg.status == 0x90);
    REQUIRE(port.sent()[1].msg.status == 0x80);
    REQUIRE(port.sent()[0].deadline == Tick{0});
    REQUIRE(port.sent()[1].deadline == Tick{100});
}

TEST_CASE("Engine plays a single placement once when no loop is set",
          "[engine][playback]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    // No loop region → placement plays once and silence after.
    eng.set_project(make_one_pattern_project(1000, {make_note(60, 0, 200)}));
    eng.transport().play();

    eng.advance(Tick{2500});

    // Exactly one on/off pair — the placement does not loop on its own.
    REQUIRE(port.sent().size() == 2);
    REQUIRE(port.sent()[0].deadline == Tick{0});
    REQUIRE(port.sent()[0].msg.status == 0x90);
    REQUIRE(port.sent()[1].deadline == Tick{200});
    REQUIRE(port.sent()[1].msg.status == 0x80);
}

TEST_CASE("Loop region wraps and force-releases held notes at the boundary",
          "[engine][loop]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    // Note that crosses the loop boundary: start=900 length=300 in a 1000-tick
    // loop → on at 900, then a forced off at 1000 (loop wrap), then second
    // iteration starts.
    eng.set_project(make_one_pattern_project(1000, {make_note(60, 900, 300)},
                                             /*loop=*/true));
    eng.transport().play();

    eng.advance(Tick{2000});

    // Expected events:
    //   - on  at 900  (first iteration)
    //   - off at 1000 (forced release on loop wrap)
    //   - on  at 900  (second iteration, but in absolute song time it's the
    //                  resumed first iteration of the loop → still tick 900)
    //   - the second-iteration off at 1200 is also forced at the next wrap
    //     when transport reaches 1000 again — which happens within this
    //     advance because we've gone 2000 ticks in song time.
    REQUIRE(port.sent().size() >= 3);
    REQUIRE(port.sent()[0].msg == MidiMessage::note_on(Channel{0}, Pitch{60}, Velocity{100}));
    REQUIRE(port.sent()[0].deadline == Tick{900});
    REQUIRE(port.sent()[1].msg.status == 0x80);  // forced off at first wrap
    REQUIRE(port.sent()[1].deadline == Tick{1000});
}

TEST_CASE("Engine::all_notes_off releases held notes", "[engine][playback]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    Project p;
    auto tid = p.add_track(Track{"t", "", Channel{0}});
    auto* track = p.find_track(tid);
    Pattern pat{"p", Tick{960}, Channel{0}};
    pat.add_note(make_note(60, 0, 800));
    pat.add_note(make_note(64, 100, 800));
    auto pid = track->add_pattern(std::move(pat));
    p.add_placement(PatternPlacement{tid, pid, Tick{0}});
    eng.set_project(p);
    eng.transport().play();

    eng.advance(Tick{200});
    REQUIRE(port.sent().size() == 2);  // two note_ons, offs pending

    eng.all_notes_off();
    REQUIRE(port.sent().size() == 4);
    REQUIRE(port.sent()[2].msg.status == 0x80);
    REQUIRE(port.sent()[3].msg.status == 0x80);
}

TEST_CASE("Engine emits multiple notes in start order", "[engine][playback]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    Project p;
    auto tid = p.add_track(Track{"t", "", Channel{0}});
    auto* track = p.find_track(tid);
    Pattern pat{"p", Tick{960}, Channel{0}};
    pat.add_note(make_note(67, 480, 100));
    pat.add_note(make_note(60, 0, 100));
    pat.add_note(make_note(64, 240, 100));
    auto pid = track->add_pattern(std::move(pat));
    p.add_placement(PatternPlacement{tid, pid, Tick{0}});
    eng.set_project(p);
    eng.transport().play();

    eng.advance(Tick{600});

    REQUIRE(port.sent().size() == 6);
    REQUIRE(port.sent()[0].msg.data1 == 60);
    REQUIRE(port.sent()[1].msg.data1 == 60);
    REQUIRE(port.sent()[2].msg.data1 == 64);
    REQUIRE(port.sent()[3].msg.data1 == 64);
    REQUIRE(port.sent()[4].msg.data1 == 67);
    REQUIRE(port.sent()[5].msg.data1 == 67);
}
