#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include <juce_audio_devices/juce_audio_devices.h>

#include <turdus/core/Channel.hpp>
#include <turdus/core/Pitch.hpp>
#include <turdus/core/Velocity.hpp>
#include <turdus/midi/JuceMidiBackend.hpp>
#include <turdus/midi/MidiBackend.hpp>
#include <turdus/midi/MidiMessage.hpp>

int main(int argc, char* argv[]) {
    juce::initialiseJuce_GUI();

    auto backend = turdus::midi::make_juce_backend();
    auto ports = backend->list_output_ports();

    std::cout << "Available MIDI output ports:\n";
    for (std::size_t i = 0; i < ports.size(); ++i) {
        std::cout << "  [" << i << "] " << ports[i] << '\n';
    }

    if (ports.empty()) {
        std::cout << "No MIDI output ports available.\n";
        juce::shutdownJuce_GUI();
        return 1;
    }

    std::size_t index = 0;
    if (argc > 1) {
        index = static_cast<std::size_t>(std::strtoul(argv[1], nullptr, 10));
    }
    if (index >= ports.size()) {
        std::cerr << "Invalid port index " << index << '\n';
        juce::shutdownJuce_GUI();
        return 1;
    }

    std::cout << "Opening port [" << index << "] " << ports[index] << '\n';
    auto port = backend->create_output_port(ports[index]);
    if (!port->open()) {
        std::cerr << "Failed to open port\n";
        juce::shutdownJuce_GUI();
        return 1;
    }

    using turdus::core::Channel;
    using turdus::core::Pitch;
    using turdus::core::Velocity;
    using turdus::midi::MidiMessage;

    std::cout << "Sending C major chord on channel 1...\n";
    const auto channel = Channel{0};
    for (int note : {60, 64, 67}) {
        port->send(MidiMessage::note_on(channel, Pitch{note}, Velocity{100}));
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    for (int note : {60, 64, 67}) {
        port->send(MidiMessage::note_off(channel, Pitch{note}));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    port->close();
    std::cout << "Done.\n";

    juce::shutdownJuce_GUI();
    return 0;
}
