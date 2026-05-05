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
#include <turdus/midi/MidiBackend.hpp>
#include <turdus/midi/MidiPort.hpp>
#include <turdus/model/Project.hpp>

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

    // ---------------------------------------------------------------- ports
    std::vector<std::string> available_ports();

    // Open the named device as the engine's output. Stops/restarts the clock thread
    // around the swap. Returns false if the device cannot be opened.
    bool set_active_port(const std::string& device_name);

    const std::string& active_port_name() const noexcept { return active_port_name_; }

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
    void install_first_pattern_into_engine();  // UI-thread side
    void stop_clock_briefly();
    void resume_clock_if_was_running(bool was_running);
};

}  // namespace turdus::app
