#include <turdus/app/AppController.hpp>

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

}  // namespace turdus::app
