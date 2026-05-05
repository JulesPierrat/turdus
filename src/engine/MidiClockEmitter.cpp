#include <turdus/engine/MidiClockEmitter.hpp>

#include <turdus/midi/MidiMessage.hpp>

namespace turdus::engine {

void MidiClockEmitter::emit_play(core::Tick from_position) {
    if (!enabled() || port_ == nullptr) {
        return;
    }
    if (from_position.value() == 0) {
        port_->send(midi::MidiMessage::start(), from_position);
    } else {
        port_->send(midi::MidiMessage::cont(), from_position);
    }
}

void MidiClockEmitter::emit_stop(core::Tick at_position) {
    if (!enabled() || port_ == nullptr) {
        return;
    }
    port_->send(midi::MidiMessage::stop(), at_position);
}

void MidiClockEmitter::emit_pulses(core::Tick from, core::Tick to) {
    if (!enabled() || port_ == nullptr || to <= from) {
        return;
    }
    constexpr auto step = kTicksPerPulse;
    const auto first = ((from.value() + step - 1) / step) * step;
    for (auto t = first; t < to.value(); t += step) {
        port_->send(midi::MidiMessage::clock(), core::Tick{t});
    }
}

}  // namespace turdus::engine
