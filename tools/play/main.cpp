#include <atomic>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include <juce_audio_devices/juce_audio_devices.h>

#include <turdus/engine/Clock.hpp>
#include <turdus/engine/Engine.hpp>
#include <turdus/io/ProjectIO.hpp>
#include <turdus/midi/JuceMidiBackend.hpp>
#include <turdus/midi/MidiBackend.hpp>

namespace {
std::atomic<bool> g_running{true};

extern "C" void on_sigint(int /*sig*/) {
    g_running.store(false, std::memory_order_release);
}
}  // namespace

int main(int argc, char* argv[]) {
    using namespace turdus;

    if (argc < 2) {
        std::cerr << "usage: " << argv[0]
                  << " <project.turdus> [port_index] [--clock]\n"
                     "  --clock   emit MIDI clock (0xF8 24 PPQN) and Start/Continue/Stop\n";
        return 1;
    }

    bool emit_clock = false;
    std::size_t port_idx = 0;
    bool port_idx_set = false;
    for (int i = 2; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--clock" || a == "-c") {
            emit_clock = true;
        } else if (!port_idx_set) {
            port_idx = static_cast<std::size_t>(std::strtoul(argv[i], nullptr, 10));
            port_idx_set = true;
        } else {
            std::cerr << "unrecognized argument: " << a << '\n';
            return 1;
        }
    }

    juce::initialiseJuce_GUI();

    auto loaded = io::ProjectIO::load(argv[1]);
    if (!loaded.ok()) {
        std::cerr << "load failed: " << loaded.detail
                  << " (" << io::describe(loaded.error) << ")\n";
        juce::shutdownJuce_GUI();
        return 1;
    }
    auto& project = *loaded.project;

    if (project.tracks().empty() || project.tracks().front().track.patterns().empty()) {
        std::cerr << "project has no tracks/patterns to play\n";
        juce::shutdownJuce_GUI();
        return 1;
    }
    const auto& track = project.tracks().front().track;
    const auto& pattern = track.patterns().front().pattern;

    auto backend = midi::make_juce_backend();
    auto ports = backend->list_output_ports();
    if (ports.empty()) {
        std::cerr << "no MIDI output ports\n";
        juce::shutdownJuce_GUI();
        return 1;
    }

    std::cout << "Available MIDI output ports:\n";
    for (std::size_t i = 0; i < ports.size(); ++i) {
        std::cout << "  [" << i << "] " << ports[i] << '\n';
    }

    if (port_idx >= ports.size()) {
        std::cerr << "invalid port index " << port_idx << '\n';
        juce::shutdownJuce_GUI();
        return 1;
    }

    std::cout << "Opening port [" << port_idx << "] " << ports[port_idx] << '\n';
    auto port = backend->create_output_port(ports[port_idx]);
    if (!port->open()) {
        std::cerr << "failed to open port\n";
        juce::shutdownJuce_GUI();
        return 1;
    }

    engine::Engine eng{port.get()};
    eng.set_pattern(pattern, track.channel());
    eng.transport().set_tempo(project.tempo());
    eng.set_clock_enabled(emit_clock);
    if (emit_clock) {
        std::cout << "MIDI clock enabled (24 PPQN + Start/Stop)\n";
    }

    engine::Clock clock;
    clock.attach(&eng);

    std::cout << "Playing '" << project.name() << "' on track '" << track.name()
              << "' / pattern '" << pattern.name() << "' (Ctrl+C to stop)...\n";
    std::signal(SIGINT, on_sigint);
    eng.transport().play();
    clock.start();

    while (g_running.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\nStopping...\n";
    clock.stop();
    eng.transport().stop();
    eng.all_notes_off();
    port->close();

    juce::shutdownJuce_GUI();
    return 0;
}
