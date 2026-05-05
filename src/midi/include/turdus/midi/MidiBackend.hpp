#pragma once

#include <memory>
#include <string>
#include <vector>

#include <turdus/midi/MidiPort.hpp>

namespace turdus::midi {

// Abstract MIDI backend: enumerates output devices and instantiates ports for them.
// The returned MidiPort is created closed — call open() on it.
class MidiBackend {
public:
    virtual ~MidiBackend() = default;

    virtual std::vector<std::string> list_output_ports() = 0;
    virtual std::unique_ptr<MidiPort> create_output_port(const std::string& name) = 0;
};

}  // namespace turdus::midi
