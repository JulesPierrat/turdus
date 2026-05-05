#include <turdus/engine/Engine.hpp>

#include <turdus/midi/MidiMessage.hpp>

namespace turdus::engine {

Engine::Engine(midi::MidiPort* port) noexcept : port_(port), clock_(port) {}

void Engine::set_pattern(const model::Pattern& pattern, core::Channel channel) {
    pattern_ = pattern;
    channel_ = channel;
}

void Engine::clear_pattern() noexcept { pattern_.reset(); }

void Engine::advance(core::Tick delta) {
    const auto current_state = transport_.state();
    const auto from = transport_.position();

    // 1. Detect transport state transitions and emit the corresponding MIDI clock
    //    messages. Stop also releases any held notes (slaves don't release notes
    //    on receiving 0xFC).
    const bool just_started =
        current_state == Transport::State::Playing && prev_state_ != Transport::State::Playing;
    const bool just_stopped =
        current_state != Transport::State::Playing && prev_state_ == Transport::State::Playing;

    if (just_started) {
        clock_.emit_play(from);
    } else if (just_stopped) {
        // `from` may already be reset to 0 by transport.stop(); use the last known
        // playback position for the deadline so the wire timestamp stays meaningful.
        const auto stop_at = last_known_position_;
        for (const auto& off : pending_offs_) {
            port_->send(midi::MidiMessage::note_off(off.channel, off.pitch), stop_at);
        }
        pending_offs_.clear();
        clock_.emit_stop(stop_at);
    }
    prev_state_ = current_state;

    if (current_state != Transport::State::Playing) {
        return;
    }

    const auto to = from + delta;

    // 2. Fire pending note_offs whose due time is before `to`.
    auto it = pending_offs_.begin();
    while (it != pending_offs_.end()) {
        if (it->due < to) {
            port_->send(midi::MidiMessage::note_off(it->channel, it->pitch), it->due);
            it = pending_offs_.erase(it);
        } else {
            ++it;
        }
    }

    // 3. MIDI clock pulses at PPQN boundaries within [from, to).
    clock_.emit_pulses(from, to);

    // 4. Pattern walk across [from, to), accounting for loops.
    if (pattern_) {
        const auto length = pattern_->length();
        if (length.value() > 0) {
            const auto iter0 = (from.value() / length.value()) * length.value();
            auto iter_start = core::Tick{iter0};
            while (iter_start < to) {
                for (const auto& entry : pattern_->notes()) {
                    const auto on_at = iter_start + entry.note.start;
                    if (on_at < from || on_at >= to) {
                        continue;
                    }
                    const auto off_at = on_at + entry.note.length;
                    port_->send(
                        midi::MidiMessage::note_on(channel_, entry.note.pitch, entry.note.velocity),
                        on_at);
                    if (off_at < to) {
                        port_->send(midi::MidiMessage::note_off(channel_, entry.note.pitch),
                                    off_at);
                    } else {
                        pending_offs_.push_back({entry.note.pitch, channel_, off_at});
                    }
                }
                iter_start = iter_start + length;
            }
        }
    }

    // 5. Update transport position and remember it for a future stop transition.
    transport_.advance(delta);
    last_known_position_ = to;
}

void Engine::all_notes_off() {
    for (const auto& off : pending_offs_) {
        port_->send(midi::MidiMessage::note_off(off.channel, off.pitch), off.due);
    }
    pending_offs_.clear();
}

}  // namespace turdus::engine
