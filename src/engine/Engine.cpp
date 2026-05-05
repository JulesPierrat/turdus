#include <turdus/engine/Engine.hpp>

#include <algorithm>
#include <utility>

#include <turdus/midi/MidiMessage.hpp>

namespace turdus::engine {

Engine::Engine(midi::MidiPort* port) noexcept : port_(port), clock_(port) {}

void Engine::set_project(model::Project project) {
    // Release any held notes from the previous project to avoid stuck notes when
    // swapping arrangements at runtime.
    force_release_all_at(last_known_position_);
    project_ = std::move(project);
    set_loop(project_->loop().start, project_->loop().end);
}

void Engine::clear_project() noexcept {
    force_release_all_at(last_known_position_);
    project_.reset();
    loop_start_ = core::Tick{0};
    loop_end_ = core::Tick{0};
}

void Engine::set_loop(core::Tick start, core::Tick end) noexcept {
    loop_start_ = start;
    loop_end_ = end;
}

void Engine::advance(core::Tick delta) {
    const auto current_state = transport_.state();
    const auto from = transport_.position();

    const bool just_started =
        current_state == Transport::State::Playing && prev_state_ != Transport::State::Playing;
    const bool just_stopped =
        current_state != Transport::State::Playing && prev_state_ == Transport::State::Playing;

    if (just_started) {
        clock_.emit_play(from);
    } else if (just_stopped) {
        force_release_all_at(last_known_position_);
        clock_.emit_stop(last_known_position_);
    }
    prev_state_ = current_state;

    if (current_state != Transport::State::Playing) {
        return;
    }

    const auto to = from + delta;

    if (loop_enabled() && from < loop_end_ && to > loop_end_) {
        // Process up to the loop point, force-release held notes at the wrap, then
        // step through any whole loop passes covered by the remainder, and finish
        // on a partial pass. Each full pass re-emits the placement's notes —
        // important for correctness when delta spans several loop iterations.
        process_range(from, loop_end_);
        force_release_all_at(loop_end_);

        const auto loop_len = loop_end_.value() - loop_start_.value();
        auto remaining = to.value() - loop_end_.value();
        if (loop_len <= 0) {
            transport_.seek(loop_start_);
            last_known_position_ = loop_start_;
            return;
        }

        while (remaining >= loop_len) {
            process_range(loop_start_, loop_end_);
            force_release_all_at(loop_end_);
            remaining -= loop_len;
        }

        const auto new_to = core::Tick{loop_start_.value() + remaining};
        process_range(loop_start_, new_to);
        transport_.seek(new_to);
        last_known_position_ = new_to;
        return;
    }

    process_range(from, to);
    transport_.advance(delta);
    last_known_position_ = to;
}

void Engine::all_notes_off() { force_release_all_at(last_known_position_); }

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void Engine::process_range(core::Tick from, core::Tick to) {
    if (port_ == nullptr) {
        return;
    }

    // 1. Fire pending note_offs whose due falls before `to`.
    auto it = pending_offs_.begin();
    while (it != pending_offs_.end()) {
        if (it->due < to) {
            port_->send(midi::MidiMessage::note_off(it->channel, it->pitch), it->due);
            it = pending_offs_.erase(it);
        } else {
            ++it;
        }
    }

    // 2. MIDI clock pulses at PPQN boundaries.
    clock_.emit_pulses(from, to);

    if (!project_) {
        return;
    }

    // 3. Walk every placement that overlaps [from, to).
    for (const auto& placement : project_->arrangement()) {
        const auto* pattern = find_pattern_by_id(placement.pattern_id);
        if (pattern == nullptr) {
            continue;
        }
        const auto* track = find_track_by_id(placement.track_id);
        if (track == nullptr) {
            continue;
        }
        const auto channel = track->channel();

        const auto p_start = placement.start;
        const auto p_end = p_start + pattern->length();
        const auto overlap_from = std::max(from, p_start);
        const auto overlap_to = std::min(to, p_end);
        if (overlap_from >= overlap_to) {
            continue;
        }

        for (const auto& entry : pattern->notes()) {
            const auto on_at = p_start + entry.note.start;
            if (on_at < overlap_from || on_at >= overlap_to) {
                continue;
            }
            const auto off_at = on_at + entry.note.length;
            port_->send(midi::MidiMessage::note_on(channel, entry.note.pitch,
                                                  entry.note.velocity),
                        on_at);
            if (off_at < to) {
                port_->send(midi::MidiMessage::note_off(channel, entry.note.pitch),
                            off_at);
            } else {
                pending_offs_.push_back({entry.note.pitch, channel, off_at});
            }
        }
    }
}

void Engine::force_release_all_at(core::Tick at) {
    if (port_ != nullptr) {
        for (const auto& off : pending_offs_) {
            port_->send(midi::MidiMessage::note_off(off.channel, off.pitch), at);
        }
    }
    pending_offs_.clear();
}

const model::Pattern* Engine::find_pattern_by_id(model::PatternId id) const {
    if (!project_) {
        return nullptr;
    }
    for (const auto& te : project_->tracks()) {
        if (const auto* p = te.track.find_pattern(id); p != nullptr) {
            return p;
        }
    }
    return nullptr;
}

const model::Track* Engine::find_track_by_id(model::TrackId id) const {
    if (!project_) {
        return nullptr;
    }
    return project_->find_track(id);
}

}  // namespace turdus::engine
