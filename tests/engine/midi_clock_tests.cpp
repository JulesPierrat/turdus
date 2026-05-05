#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include <turdus/core/Bpm.hpp>
#include <turdus/core/Channel.hpp>
#include <turdus/core/Pitch.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/core/Velocity.hpp>
#include <turdus/engine/Engine.hpp>
#include <turdus/engine/MidiClockEmitter.hpp>
#include <turdus/midi/FakeMidiPort.hpp>
#include <turdus/midi/MidiMessage.hpp>
#include <turdus/model/Note.hpp>
#include <turdus/model/Pattern.hpp>

using namespace turdus::core;
using namespace turdus::engine;
using namespace turdus::midi;
using namespace turdus::model;

namespace {
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

// ---------------------------------------------------------------------------
// Pure MidiClockEmitter unit tests
// ---------------------------------------------------------------------------

TEST_CASE("MidiClockEmitter::kTicksPerPulse is 40 (PPQ 960 / 24 PPQN)",
          "[engine][clock_emit]") {
    REQUIRE(MidiClockEmitter::kTicksPerPulse == 40);
}

TEST_CASE("Disabled emitter is silent", "[engine][clock_emit]") {
    FakeMidiPort port{"test"};
    port.open();
    MidiClockEmitter emit{&port};
    REQUIRE_FALSE(emit.enabled());

    emit.emit_play(Tick{0});
    emit.emit_pulses(Tick{0}, Tick{1000});
    emit.emit_stop(Tick{500});
    REQUIRE(port.sent().empty());
}

TEST_CASE("Enabled emitter sends Start when from position 0", "[engine][clock_emit]") {
    FakeMidiPort port{"test"};
    port.open();
    MidiClockEmitter emit{&port};
    emit.set_enabled(true);

    emit.emit_play(Tick{0});
    REQUIRE(port.sent().size() == 1);
    REQUIRE(port.sent()[0].msg == MidiMessage::start());
    REQUIRE(port.sent()[0].deadline == Tick{0});
}

TEST_CASE("Enabled emitter sends Continue when from non-zero position",
          "[engine][clock_emit]") {
    FakeMidiPort port{"test"};
    port.open();
    MidiClockEmitter emit{&port};
    emit.set_enabled(true);

    emit.emit_play(Tick{960});
    REQUIRE(port.sent().size() == 1);
    REQUIRE(port.sent()[0].msg == MidiMessage::cont());
    REQUIRE(port.sent()[0].deadline == Tick{960});
}

TEST_CASE("emit_pulses emits every 40 ticks within the range", "[engine][clock_emit]") {
    FakeMidiPort port{"test"};
    port.open();
    MidiClockEmitter emit{&port};
    emit.set_enabled(true);

    // [0, 200): expect pulses at 0, 40, 80, 120, 160 → 5 pulses
    emit.emit_pulses(Tick{0}, Tick{200});
    REQUIRE(port.sent().size() == 5);
    for (std::size_t i = 0; i < 5; ++i) {
        REQUIRE(port.sent()[i].msg == MidiMessage::clock());
        REQUIRE(port.sent()[i].deadline == Tick{static_cast<std::int64_t>(i * 40)});
    }
}

TEST_CASE("emit_pulses snaps the first pulse to the next 40-multiple",
          "[engine][clock_emit]") {
    FakeMidiPort port{"test"};
    port.open();
    MidiClockEmitter emit{&port};
    emit.set_enabled(true);

    // [50, 200): first pulse should be at 80 (smallest multiple of 40 ≥ 50).
    emit.emit_pulses(Tick{50}, Tick{200});
    REQUIRE(port.sent().size() == 3);
    REQUIRE(port.sent()[0].deadline == Tick{80});
    REQUIRE(port.sent()[1].deadline == Tick{120});
    REQUIRE(port.sent()[2].deadline == Tick{160});
}

TEST_CASE("emit_pulses produces 24 pulses per quarter note", "[engine][clock_emit]") {
    FakeMidiPort port{"test"};
    port.open();
    MidiClockEmitter emit{&port};
    emit.set_enabled(true);

    // One quarter note = 960 ticks. Range [0, 960) → 24 pulses.
    emit.emit_pulses(Tick{0}, Tick{960});
    REQUIRE(port.sent().size() == 24);
    REQUIRE(port.sent().front().deadline == Tick{0});
    REQUIRE(port.sent().back().deadline == Tick{920});  // 24th pulse: 23 * 40
}

// ---------------------------------------------------------------------------
// Engine integration: clock emission is wired correctly
// ---------------------------------------------------------------------------

TEST_CASE("Engine clock disabled by default — no real-time bytes emitted",
          "[engine][clock_integration]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.transport().play();
    eng.advance(Tick{1000});
    REQUIRE(count_status(port, 0xF8) == 0);
    REQUIRE(count_status(port, 0xFA) == 0);
    REQUIRE(count_status(port, 0xFC) == 0);
}

TEST_CASE("Engine emits Start (0xFA) on first advance after play from position 0",
          "[engine][clock_integration]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.set_clock_enabled(true);
    eng.transport().play();

    eng.advance(Tick{1});  // tiny advance, enough to detect transition

    REQUIRE(count_status(port, 0xFA) == 1);
    // Start must be the very first byte the slave sees.
    REQUIRE(port.sent().front().msg == MidiMessage::start());
}

TEST_CASE("Engine emits Continue (0xFB) when resuming from a non-zero position",
          "[engine][clock_integration]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.set_clock_enabled(true);
    eng.transport().seek(Tick{500});
    eng.transport().play();

    eng.advance(Tick{1});

    REQUIRE(count_status(port, 0xFB) == 1);
    REQUIRE(count_status(port, 0xFA) == 0);
    REQUIRE(port.sent().front().msg == MidiMessage::cont());
}

TEST_CASE("Engine emits Stop (0xFC) on Playing → Stopped transition",
          "[engine][clock_integration]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.set_clock_enabled(true);
    eng.transport().play();
    eng.advance(Tick{200});  // some playback happens, last_known_position = 200
    port.clear();

    eng.transport().stop();   // resets position to 0
    eng.advance(Tick{1});     // detect transition

    REQUIRE(count_status(port, 0xFC) == 1);
    auto stop_idx =
        std::find_if(port.sent().begin(), port.sent().end(),
                     [](const FakeMidiPort::SentMessage& s) { return s.msg.status == 0xFC; });
    REQUIRE(stop_idx != port.sent().end());
    // Stop deadline carries the last known playback position, not the post-reset 0.
    REQUIRE(stop_idx->deadline == Tick{200});
}

TEST_CASE("Stop releases held notes BEFORE emitting 0xFC",
          "[engine][clock_integration]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.set_clock_enabled(true);

    Project project;
    auto tid = project.add_track(Track{"t", "", Channel{0}});
    auto* track = project.find_track(tid);
    Pattern pat{"test", Tick{960}, Channel{0}};
    pat.add_note(Note{Pitch{60}, Tick{0}, Tick{800}, Velocity{100}});
    auto pid = track->add_pattern(std::move(pat));
    project.add_placement(PatternPlacement{tid, pid, Tick{0}});
    eng.set_project(project);

    eng.transport().play();
    eng.advance(Tick{200});  // emits Start, clock pulses, note_on; off pending
    port.clear();

    eng.transport().stop();
    eng.advance(Tick{1});

    // Order: note_off then 0xFC. Slaves don't release notes on Stop, so the order matters.
    REQUIRE(port.sent().size() >= 2);
    REQUIRE(port.sent()[0].msg.status == 0x80);  // note_off
    REQUIRE(port.sent().back().msg == MidiMessage::stop());
}

TEST_CASE("Engine emits 24 PPQN clock pulses while playing",
          "[engine][clock_integration]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.set_clock_enabled(true);
    eng.transport().play();

    // 1 beat = 960 ticks → 24 clock pulses.
    eng.advance(Tick{960});

    REQUIRE(count_status(port, 0xFA) == 1);  // Start once
    REQUIRE(count_status(port, 0xF8) == 24);  // 24 pulses per quarter note
}

TEST_CASE("Clock pulse count matches across multiple advances",
          "[engine][clock_integration]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.set_clock_enabled(true);
    eng.transport().play();

    // Drip-feed advances summing to 4 quarter notes.
    for (int i = 0; i < 100; ++i) {
        eng.advance(Tick{38});  // arbitrary non-multiple of 40
    }
    eng.advance(Tick{4 * 960 - 100 * 38});  // top up to exactly 4 beats

    REQUIRE(count_status(port, 0xF8) == 4 * 24);
}

TEST_CASE("Tempo change does not affect tick math (engine sees ticks, not time)",
          "[engine][clock_integration]") {
    FakeMidiPort port{"test"};
    port.open();
    Engine eng{&port};
    eng.set_clock_enabled(true);
    eng.transport().set_tempo(Bpm{200.0});
    eng.transport().play();

    eng.advance(Tick{960});
    REQUIRE(count_status(port, 0xF8) == 24);
}
