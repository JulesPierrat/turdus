#include <catch2/catch_test_macros.hpp>

#include <algorithm>

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

Note make_note(int pitch, int start, int length, int vel = 100) {
    return Note{Pitch{pitch}, Tick{start}, Tick{length}, Velocity{vel}};
}

std::size_t count_status(const FakeMidiPort& port, std::uint8_t status) {
    std::size_t n = 0;
    for (const auto& s : port.sent()) {
        if (s.msg.status == status) {
            ++n;
        }
    }
    return n;
}

}  // namespace

TEST_CASE("Engine plays multi-track arrangement with per-track channel routing",
          "[engine][phase9][arrangement]") {
    FakeMidiPort port{"test"};
    port.open();

    Project project;
    auto t1 = project.add_track(Track{"Lead", "", Channel{0}});
    auto t2 = project.add_track(Track{"Drums", "", Channel{9}});

    auto* tr1 = project.find_track(t1);
    auto* tr2 = project.find_track(t2);

    Pattern lead{"Lead-pat", Tick{960}, Channel{0}};
    lead.add_note(make_note(60, 0, 240));
    auto lp = tr1->add_pattern(std::move(lead));

    Pattern drums{"Drum-pat", Tick{480}, Channel{9}};
    drums.add_note(make_note(36, 0, 100));
    auto dp = tr2->add_pattern(std::move(drums));

    project.add_placement(PatternPlacement{t1, lp, Tick{0}});
    project.add_placement(PatternPlacement{t2, dp, Tick{0}});

    Engine eng{&port};
    eng.set_project(project);
    eng.transport().play();

    eng.advance(Tick{1000});

    // Lead note → channel 0, status 0x90/0x80
    // Drums note → channel 9, status 0x99/0x89
    REQUIRE(count_status(port, 0x90) == 1);  // Lead note_on
    REQUIRE(count_status(port, 0x80) == 1);  // Lead note_off
    REQUIRE(count_status(port, 0x99) == 1);  // Drums note_on
    REQUIRE(count_status(port, 0x89) == 1);  // Drums note_off
}

TEST_CASE("Engine respects placement start offsets",
          "[engine][phase9][arrangement]") {
    FakeMidiPort port{"test"};
    port.open();

    Project project;
    auto tid = project.add_track(Track{"t", "", Channel{0}});
    auto* track = project.find_track(tid);

    Pattern p{"p", Tick{200}, Channel{0}};
    p.add_note(make_note(60, 0, 100));
    auto pid = track->add_pattern(std::move(p));

    // Three placements: t=0, t=500, t=1000
    project.add_placement(PatternPlacement{tid, pid, Tick{0}});
    project.add_placement(PatternPlacement{tid, pid, Tick{500}});
    project.add_placement(PatternPlacement{tid, pid, Tick{1000}});

    Engine eng{&port};
    eng.set_project(project);
    eng.transport().play();

    eng.advance(Tick{1500});

    REQUIRE(port.sent().size() == 6);  // 3 on + 3 off
    REQUIRE(port.sent()[0].deadline == Tick{0});      // first on
    REQUIRE(port.sent()[1].deadline == Tick{100});    // first off
    REQUIRE(port.sent()[2].deadline == Tick{500});    // second on
    REQUIRE(port.sent()[3].deadline == Tick{600});    // second off
    REQUIRE(port.sent()[4].deadline == Tick{1000});   // third on
    REQUIRE(port.sent()[5].deadline == Tick{1100});   // third off
}

TEST_CASE("Loop region wraps song time", "[engine][phase9][loop]") {
    FakeMidiPort port{"test"};
    port.open();

    Project project;
    auto tid = project.add_track(Track{"t", "", Channel{0}});
    auto* track = project.find_track(tid);

    // Pattern with one note at the start.
    Pattern p{"p", Tick{500}, Channel{0}};
    p.add_note(make_note(60, 0, 100));
    auto pid = track->add_pattern(std::move(p));
    project.add_placement(PatternPlacement{tid, pid, Tick{0}});

    // 500-tick loop.
    project.set_loop(LoopRegion{Tick{0}, Tick{500}});

    Engine eng{&port};
    eng.set_project(project);
    eng.transport().play();

    // Advance through 3 loop iterations.
    eng.advance(Tick{1500});

    // Each iteration: on at song-tick 0 (relative), off at 100. With force-release
    // at wrap, the note off has already fired by 100 well before any wrap. Each
    // iteration produces exactly one on + one off. Total: 3 of each.
    REQUIRE(count_status(port, 0x90) == 3);
    REQUIRE(count_status(port, 0x80) == 3);
}

TEST_CASE("Loop region force-releases held notes at the wrap",
          "[engine][phase9][loop]") {
    FakeMidiPort port{"test"};
    port.open();

    Project project;
    auto tid = project.add_track(Track{"t", "", Channel{0}});
    auto* track = project.find_track(tid);

    // Note that would extend past the loop end.
    Pattern p{"p", Tick{500}, Channel{0}};
    p.add_note(make_note(60, 400, 300));  // off at 700 — past loop_end=500
    auto pid = track->add_pattern(std::move(p));
    project.add_placement(PatternPlacement{tid, pid, Tick{0}});

    project.set_loop(LoopRegion{Tick{0}, Tick{500}});

    Engine eng{&port};
    eng.set_project(project);
    eng.transport().play();

    eng.advance(Tick{600});

    // Expected: on at 400, forced off at 500 (loop wrap), then on at 400 again
    // in the next iteration (after wrap, song tick 0 + 100 = position 100,
    // which is before the next note starts).
    REQUIRE(port.sent().size() >= 2);
    REQUIRE(port.sent()[0].msg.status == 0x90);
    REQUIRE(port.sent()[0].deadline == Tick{400});
    REQUIRE(port.sent()[1].msg.status == 0x80);
    REQUIRE(port.sent()[1].deadline == Tick{500});  // forced at wrap
}

TEST_CASE("set_project adopts the project's loop region",
          "[engine][phase9][loop]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    REQUIRE_FALSE(eng.loop_enabled());

    Project project;
    project.set_loop(LoopRegion{Tick{100}, Tick{500}});
    eng.set_project(project);

    REQUIRE(eng.loop_enabled());
    REQUIRE(eng.loop_start() == Tick{100});
    REQUIRE(eng.loop_end() == Tick{500});
}

TEST_CASE("set_loop overrides without modifying the project",
          "[engine][phase9][loop]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};

    Project project;
    project.set_loop(LoopRegion{Tick{0}, Tick{500}});
    eng.set_project(project);

    eng.set_loop(Tick{200}, Tick{800});
    REQUIRE(eng.loop_start() == Tick{200});
    REQUIRE(eng.loop_end() == Tick{800});
}

TEST_CASE("clear_project releases held notes",
          "[engine][phase9][lifecycle]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};

    Project project;
    auto tid = project.add_track(Track{"t", "", Channel{0}});
    auto* track = project.find_track(tid);
    Pattern p{"p", Tick{1000}, Channel{0}};
    p.add_note(make_note(60, 0, 800));
    auto pid = track->add_pattern(std::move(p));
    project.add_placement(PatternPlacement{tid, pid, Tick{0}});
    eng.set_project(project);
    eng.transport().play();

    eng.advance(Tick{200});  // note_on at 0; off at 800 pending
    port.clear();

    eng.clear_project();

    // Pending off should have fired during clear_project.
    REQUIRE(port.sent().size() == 1);
    REQUIRE(port.sent()[0].msg.status == 0x80);
}
