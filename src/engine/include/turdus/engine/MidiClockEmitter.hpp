#pragma once

#include <turdus/core/Tick.hpp>
#include <turdus/midi/MidiPort.hpp>

namespace turdus::engine {

// Emits MIDI Real-Time messages: Clock (0xF8) at 24 PPQN, plus Start (0xFA),
// Continue (0xFB) and Stop (0xFC) transport messages. Driven by the Engine via
// dedicated hooks; itself stateless beyond the enabled flag.
class MidiClockEmitter {
public:
    // 24 PPQN out of a 960 PPQ engine resolution → one pulse every 40 ticks.
    static constexpr core::Tick::value_type kTicksPerPulse = 40;

    explicit MidiClockEmitter(midi::MidiPort* port) noexcept : port_(port) {}

    void set_enabled(bool e) noexcept { enabled_ = e; }
    bool enabled() const noexcept { return enabled_; }

    // Transport just transitioned Stopped → Playing. Emits Start (0xFA) if
    // resuming from position 0, Continue (0xFB) otherwise.
    void emit_play(core::Tick from_position);

    // Transport just transitioned Playing → Stopped. Emits Stop (0xFC).
    void emit_stop(core::Tick at_position);

    // Emit Clock (0xF8) at every PPQN boundary in [from, to).
    void emit_pulses(core::Tick from, core::Tick to);

private:
    midi::MidiPort* port_;
    bool enabled_{false};
};

}  // namespace turdus::engine
