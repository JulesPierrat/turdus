#include <turdus/app/AppController.hpp>

#include <algorithm>
#include <utility>

#include <turdus/io/ProjectIO.hpp>

namespace turdus::app {

AppController::AppController(std::unique_ptr<midi::MidiBackend> backend)
    : backend_(std::move(backend)),
      bus_(),
      engine_(nullptr),  // port set later via set_active_port
      clock_() {
    clock_.attach(&engine_);
}

AppController::~AppController() {
    clock_.stop();
    if (active_port_) {
        active_port_->close();
    }
}

// ----- transport ----------------------------------------------------------------
// All Transport mutations are atomic, so we forward them directly without a queue.
// Phase 8 will introduce the command bus for non-trivial edits (note ops).

void AppController::play() { engine_.transport().play(); }
void AppController::stop() { engine_.transport().stop(); }

void AppController::set_tempo(core::Bpm tempo) { engine_.transport().set_tempo(tempo); }

void AppController::set_clock_enabled(bool enabled) { engine_.set_clock_enabled(enabled); }

bool AppController::is_playing() const noexcept { return engine_.transport().is_playing(); }
core::Tick AppController::position() const noexcept { return engine_.transport().position(); }
core::Bpm AppController::tempo() const noexcept { return engine_.transport().tempo(); }

// ----- projects -----------------------------------------------------------------

void AppController::new_project() {
    const bool was_running = clock_.is_running();
    if (was_running) {
        clock_.stop();
    }
    project_ = model::Project{};
    engine_.transport().stop();
    engine_.clear_pattern();
    if (was_running) {
        clock_.start();
    }
}

bool AppController::open_project(const std::filesystem::path& path) {
    auto loaded = io::ProjectIO::load(path);
    if (!loaded.ok()) {
        return false;
    }

    const bool was_running = clock_.is_running();
    if (was_running) {
        clock_.stop();
    }

    project_ = std::move(*loaded.project);
    engine_.transport().stop();
    install_first_pattern_into_engine();
    engine_.transport().set_tempo(project_.tempo());

    if (was_running) {
        clock_.start();
    }
    return true;
}

bool AppController::save_project(const std::filesystem::path& path) {
    return io::ProjectIO::save(project_, path).ok();
}

void AppController::install_first_pattern_into_engine() {
    if (project_.tracks().empty()
        || project_.tracks().front().track.patterns().empty()) {
        engine_.clear_pattern();
        return;
    }
    const auto& track = project_.tracks().front().track;
    const auto& pattern = track.patterns().front().pattern;
    engine_.set_pattern(pattern, track.channel());
}

// ----- ports --------------------------------------------------------------------

std::vector<std::string> AppController::available_ports() {
    return backend_ != nullptr ? backend_->list_output_ports() : std::vector<std::string>{};
}

bool AppController::set_active_port(const std::string& device_name) {
    const bool was_running = clock_.is_running();
    if (was_running) {
        clock_.stop();
    }

    // Detach engine from the old port first so a failed open doesn't leave a
    // dangling pointer hanging around if a final advance() were to fire.
    engine_.set_port(nullptr);
    if (active_port_) {
        active_port_->close();
        active_port_.reset();
    }

    if (backend_ != nullptr && !device_name.empty()) {
        auto port = backend_->create_output_port(device_name);
        if (port && port->open()) {
            active_port_ = std::move(port);
            active_port_name_ = device_name;
            engine_.set_port(active_port_.get());
        } else {
            active_port_name_.clear();
        }
    } else {
        active_port_name_.clear();
    }

    if (was_running && active_port_) {
        clock_.start();
    }
    return active_port_ != nullptr;
}

void AppController::stop_clock_briefly() {
    if (clock_.is_running()) {
        clock_.stop();
    }
}

void AppController::resume_clock_if_was_running(bool was_running) {
    if (was_running && active_port_) {
        clock_.start();
    }
}

void AppController::apply_pending_commands() {
    // Reserved for Phase 8: the command bus drains here.
    Command cmd;
    while (bus_.try_pop(cmd)) {
        // No-op for Phase 6 — left as a hook for future commands.
    }
}

// ----- note edits ---------------------------------------------------------------

bool AppController::is_engine_pattern(model::TrackId track_id,
                                      model::PatternId pattern_id) const noexcept {
    if (project_.tracks().empty()) {
        return false;
    }
    const auto& first_track = project_.tracks().front();
    if (first_track.id != track_id) {
        return false;
    }
    if (first_track.track.patterns().empty()) {
        return false;
    }
    return first_track.track.patterns().front().id == pattern_id;
}

model::Pattern* AppController::find_pattern(model::TrackId track_id,
                                            model::PatternId pattern_id) {
    auto* track = project_.find_track(track_id);
    return track != nullptr ? track->find_pattern(pattern_id) : nullptr;
}

void AppController::resync_engine_if_needed(model::TrackId track_id,
                                            model::PatternId pattern_id) {
    if (!is_engine_pattern(track_id, pattern_id)) {
        return;
    }
    const bool was_running = clock_.is_running();
    if (was_running) {
        clock_.stop();
    }
    install_first_pattern_into_engine();
    if (was_running) {
        clock_.start();
    }
}

model::NoteId AppController::add_note(model::TrackId track_id,
                                      model::PatternId pattern_id,
                                      model::Note note) {
    auto* pattern = find_pattern(track_id, pattern_id);
    if (pattern == nullptr) {
        return model::NoteId{};
    }
    const auto id = pattern->add_note(note);
    resync_engine_if_needed(track_id, pattern_id);
    return id;
}

bool AppController::remove_note(model::TrackId track_id,
                                model::PatternId pattern_id,
                                model::NoteId note_id) {
    auto* pattern = find_pattern(track_id, pattern_id);
    if (pattern == nullptr) {
        return false;
    }
    if (!pattern->remove_note(note_id)) {
        return false;
    }
    resync_engine_if_needed(track_id, pattern_id);
    return true;
}

bool AppController::move_note(model::TrackId track_id,
                              model::PatternId pattern_id,
                              model::NoteId note_id,
                              core::Tick new_start,
                              core::Pitch new_pitch) {
    auto* pattern = find_pattern(track_id, pattern_id);
    if (pattern == nullptr) {
        return false;
    }
    auto found = pattern->find_note(note_id);
    if (!found) {
        return false;
    }
    auto updated = *found;
    updated.start = core::Tick{std::max<std::int64_t>(0, new_start.value())};
    updated.pitch = new_pitch;
    if (!pattern->update_note(note_id, updated)) {
        return false;
    }
    resync_engine_if_needed(track_id, pattern_id);
    return true;
}

bool AppController::resize_note(model::TrackId track_id,
                                model::PatternId pattern_id,
                                model::NoteId note_id,
                                core::Tick new_length) {
    auto* pattern = find_pattern(track_id, pattern_id);
    if (pattern == nullptr) {
        return false;
    }
    auto found = pattern->find_note(note_id);
    if (!found) {
        return false;
    }
    auto updated = *found;
    updated.length = core::Tick{std::max<std::int64_t>(1, new_length.value())};
    if (!pattern->update_note(note_id, updated)) {
        return false;
    }
    resync_engine_if_needed(track_id, pattern_id);
    return true;
}

bool AppController::set_note_velocity(model::TrackId track_id,
                                      model::PatternId pattern_id,
                                      model::NoteId note_id,
                                      core::Velocity new_velocity) {
    auto* pattern = find_pattern(track_id, pattern_id);
    if (pattern == nullptr) {
        return false;
    }
    auto found = pattern->find_note(note_id);
    if (!found) {
        return false;
    }
    auto updated = *found;
    updated.velocity = new_velocity;
    if (!pattern->update_note(note_id, updated)) {
        return false;
    }
    resync_engine_if_needed(track_id, pattern_id);
    return true;
}

}  // namespace turdus::app
