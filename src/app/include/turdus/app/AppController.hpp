#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <turdus/app/CommandBus.hpp>
#include <turdus/core/Bpm.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/engine/Clock.hpp>
#include <turdus/engine/Engine.hpp>
#include <turdus/core/Pitch.hpp>
#include <turdus/core/Velocity.hpp>
#include <turdus/midi/MidiBackend.hpp>
#include <turdus/midi/MidiPort.hpp>
#include <turdus/model/Note.hpp>
#include <turdus/model/Pattern.hpp>
#include <turdus/model/Project.hpp>
#include <turdus/model/Track.hpp>

namespace turdus::app {

// Orchestrates the application: owns the project (UI-thread snapshot), the active
// MIDI port, the engine, and the clock thread. Exposes a high-level API for the
// UI, and internally drives the command bus on the engine side.
//
// Threading: most public methods are UI-thread only. Atomic transport reads (state,
// position, tempo) are safe from any thread.
class AppController {
public:
    explicit AppController(std::unique_ptr<midi::MidiBackend> backend);
    ~AppController();

    AppController(const AppController&) = delete;
    AppController& operator=(const AppController&) = delete;

    // ---------------------------------------------------------------- transport
    // These push commands through the bus (engine thread will pick them up).
    void play();
    void stop();
    void set_tempo(core::Bpm tempo);
    void set_clock_enabled(bool enabled);

    // ---------------------------------------------------------------- projects
    bool open_project(const std::filesystem::path& path);
    bool save_project(const std::filesystem::path& path);
    void new_project();

    // Loop region. start == end disables looping. Updates both the project
    // (so it persists in saved files) and the engine.
    void set_loop(core::Tick start, core::Tick end);
    core::Tick loop_start() const noexcept { return project_.loop().start; }
    core::Tick loop_end() const noexcept { return project_.loop().end; }

    // ---------------------------------------------------------------- ports
    std::vector<std::string> available_ports();

    // Open the named device as the engine's output. Stops/restarts the clock thread
    // around the swap. Returns false if the device cannot be opened.
    bool set_active_port(const std::string& device_name);

    const std::string& active_port_name() const noexcept { return active_port_name_; }

    // ---------------------------------------------------------------- note edits
    // All mutators target a specific (track, pattern). They return false if the
    // track or pattern can't be found, the id collides, or any other validation
    // fails — caller can treat as no-op.
    //
    // After a successful mutation, if the affected pattern is the one currently
    // bound to the engine (Phase 8: track 0 / pattern 0), we briefly stop the
    // clock, reload the pattern, and restart. Edits to other patterns don't touch
    // the engine.

    // Adds a note. Returns the new note's id (invalid id on failure).
    model::NoteId add_note(model::TrackId, model::PatternId, model::Note);

    bool remove_note(model::TrackId, model::PatternId, model::NoteId);

    // Move a note in time and/or pitch. Other fields preserved.
    bool move_note(model::TrackId, model::PatternId, model::NoteId,
                   core::Tick new_start, core::Pitch new_pitch);

    // Resize a note. Length is clamped to >= 1 tick.
    bool resize_note(model::TrackId, model::PatternId, model::NoteId,
                     core::Tick new_length);

    bool set_note_velocity(model::TrackId, model::PatternId, model::NoteId,
                           core::Velocity);

    // ---------------------------------------------------------------- read-only
    const model::Project& project() const noexcept { return project_; }
    bool is_playing() const noexcept;
    core::Tick position() const noexcept;
    core::Bpm tempo() const noexcept;
    bool clock_enabled() const noexcept { return engine_.clock_enabled(); }

private:
    std::unique_ptr<midi::MidiBackend> backend_;
    std::unique_ptr<midi::MidiPort> active_port_;
    std::string active_port_name_;

    model::Project project_;

    CommandBus bus_;

    engine::Engine engine_;
    engine::Clock clock_;

    void apply_pending_commands();           // engine-thread side
    void install_project_into_engine();        // UI-thread side
    void stop_clock_briefly();
    void resume_clock_if_was_running(bool was_running);

    // True if (track, pattern) is the one the engine is currently configured to
    // play. In Phase 8 v0 this is always (first track, first pattern).
    bool is_engine_pattern(model::TrackId, model::PatternId) const noexcept;

    // Locate a Pattern in the project (mutable). Returns nullptr if not found.
    model::Pattern* find_pattern(model::TrackId, model::PatternId);

    // After a mutation to a pattern, if it's the engine pattern, push the new
    // content to the engine. Briefly stops the clock if it was running.
    void resync_engine_if_needed(model::TrackId, model::PatternId);
};

}  // namespace turdus::app
