#pragma once

#include <memory>

#include <turdus/midi/MidiBackend.hpp>

// JUCE-clean public header: the only symbol exposed is the factory. The concrete
// backend type lives entirely in the .cpp so consumers don't need <juce_audio_devices/...>.

namespace turdus::midi {

std::unique_ptr<MidiBackend> make_juce_backend();

}  // namespace turdus::midi
