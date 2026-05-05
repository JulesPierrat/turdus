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
#include <turdus/model/Project.hpp>
#include <turdus/model/Track.hpp>

namespace turdus::engine {

// Phase 9 engine: arrangement-based playback. The engine holds a snapshot of a
// `Project` and walks its `arrangement` field across the tick range it advances
// through. Each `PatternPlacement` plays its referenced pattern exactly once,
// from `start` to `start + pattern.length()`. The transport position runs as
// "song time" — purely linear unless a loop region is active, in which case the
// engine wraps from `loop_end` back to `loop_start` and force-releases held
// notes at the wrap point.
//
// Phase 9 v0 limitations:
//   - single output port (multi-port routing → Phase 9.5)
//   - per-track channel via `Track::channel()`; transpose / mute / solo ignored
//   - clock fanout to a single port; multi-port clock → Phase 9.5
class Engine {
public:
    explicit Engine(midi::MidiPort* port) noexcept;

    Transport& transport() noexcept { return transport_; }
    const Transport& transport() const noexcept { return transport_; }

    // Hot port swap. Caller must ensure no concurrent advance().
    void set_port(midi::MidiPort* port) noexcept {
        port_ = port;
        clock_.set_port(port);
    }

    // Replace the engine's project snapshot. Holds a deep copy. Force-releases
    // any held notes from a previous project to avoid stuck notes. Adopts the
    // project's loop region.
    void set_project(model::Project project);
    void clear_project() noexcept;

    // Override the loop region without touching the project. start == end → off.
    void set_loop(core::Tick start, core::Tick end) noexcept;
    core::Tick loop_start() const noexcept { return loop_start_; }
    core::Tick loop_end() const noexcept { return loop_end_; }
    bool loop_enabled() const noexcept { return loop_end_ > loop_start_; }

    void set_clock_enabled(bool e) noexcept { clock_.set_enabled(e); }
    bool clock_enabled() const noexcept { return clock_.enabled(); }

    // Advance transport by `delta` ticks. Emits any MIDI events whose deadline
    // falls in the new tick range. If a loop wrap occurs, held notes are
    // force-released at `loop_end` before processing [loop_start, ...).
    void advance(core::Tick delta);

    // Emit note_off for every held note. Used for panic / before stop.
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

    std::optional<model::Project> project_;

    std::vector<PendingOff> pending_offs_;

    Transport::State prev_state_{Transport::State::Stopped};
    core::Tick last_known_position_{0};

    core::Tick loop_start_{0};
    core::Tick loop_end_{0};

    void process_range(core::Tick from, core::Tick to);
    void force_release_all_at(core::Tick at);

    const model::Pattern* find_pattern_by_id(model::PatternId id) const;
    const model::Track* find_track_by_id(model::TrackId id) const;
};

}  // namespace turdus::engine
