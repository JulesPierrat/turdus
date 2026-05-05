#pragma once

#include <optional>
#include <vector>

#include <turdus/core/Channel.hpp>
#include <turdus/core/Pitch.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/engine/MidiClockEmitter.hpp>
#include <turdus/engine/Transport.hpp>
#include <turdus/midi/MidiPort.hpp>
#include <turdus/model/Pattern.hpp>

namespace turdus::engine {

// Engine v0 — Phase 4 + Phase 5 scope:
//   - one pattern, one MIDI port
//   - loops the pattern indefinitely while transport is Playing
//   - emits matching note_off bookkeeping for every note_on
//   - optionally emits MIDI clock (0xF8) at 24 PPQN plus Start/Continue/Stop
//
// Driven by calling advance(delta) — typically from the Clock thread, but tests can
// call it directly with synthetic deltas for deterministic playback.
class Engine {
public:
    explicit Engine(midi::MidiPort* port) noexcept;

    Transport& transport() noexcept { return transport_; }
    const Transport& transport() const noexcept { return transport_; }

    void set_pattern(const model::Pattern& pattern, core::Channel channel);
    void clear_pattern() noexcept;

    void set_clock_enabled(bool e) noexcept { clock_.set_enabled(e); }
    bool clock_enabled() const noexcept { return clock_.enabled(); }

    // Advance transport by `delta` ticks and emit any MIDI events whose deadline
    // falls in the new tick range. Detects transport state transitions and emits
    // the appropriate MIDI clock messages on the same call.
    void advance(core::Tick delta);

    // Emit note_off for every note currently held. Call before stopping or as a
    // panic to release stuck notes.
    void all_notes_off();

private:
    struct PendingOff {
        core::Pitch pitch;
        core::Channel channel;
        core::Tick due;
    };

    Transport transport_;
    midi::MidiPort* port_;
    MidiClockEmitter clock_;

    std::optional<model::Pattern> pattern_;
    core::Channel channel_;

    std::vector<PendingOff> pending_offs_;

    Transport::State prev_state_{Transport::State::Stopped};
    core::Tick last_known_position_{0};
};

}  // namespace turdus::engine
