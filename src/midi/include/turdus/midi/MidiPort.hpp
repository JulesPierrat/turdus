#pragma once

#include <string>

#include <turdus/core/Tick.hpp>
#include <turdus/midi/MidiMessage.hpp>

namespace turdus::midi {

// Abstract MIDI output port. Implementations may be JUCE-backed (real hardware/virtual)
// or fake (test recording). A port is created closed; call open() before sending.
class MidiPort {
public:
    virtual ~MidiPort() = default;

    virtual const std::string& name() const = 0;

    // Returns false if the port could not be opened (device gone, name not found, etc.).
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool is_open() const = 0;

    // `deadline` is the tick at which the message should ideally fire. Backends that
    // support timestamped delivery honor it; the default JUCE backend in Phase 3
    // sends immediately. Phase 4 will wire deadlines into the engine.
    virtual void send(const MidiMessage& msg, core::Tick deadline = core::Tick{0}) = 0;
};

}  // namespace turdus::midi
