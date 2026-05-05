#pragma once

#include <cstdint>

#include <turdus/core/Channel.hpp>
#include <turdus/core/Pitch.hpp>
#include <turdus/core/Velocity.hpp>

namespace turdus::midi {

// A 1- to 3-byte MIDI message in wire format (no timestamp).
// Construct via the named factories below — they encode the status byte correctly,
// including the channel for channel-voice messages.
struct MidiMessage {
    // Channel-voice status bytes (low nibble = channel)
    static constexpr std::uint8_t kStatusNoteOff       = 0x80;
    static constexpr std::uint8_t kStatusNoteOn        = 0x90;
    static constexpr std::uint8_t kStatusCC            = 0xB0;
    static constexpr std::uint8_t kStatusProgramChange = 0xC0;

    // System real-time messages (single-byte)
    static constexpr std::uint8_t kStatusClock    = 0xF8;
    static constexpr std::uint8_t kStatusStart    = 0xFA;
    static constexpr std::uint8_t kStatusContinue = 0xFB;
    static constexpr std::uint8_t kStatusStop     = 0xFC;

    std::uint8_t status{0};
    std::uint8_t data1{0};
    std::uint8_t data2{0};
    std::uint8_t length{0};  // 1, 2, or 3

    static MidiMessage note_on(core::Channel ch, core::Pitch p, core::Velocity v) noexcept;
    static MidiMessage note_off(core::Channel ch, core::Pitch p,
                                core::Velocity v = core::Velocity{0}) noexcept;
    static MidiMessage cc(core::Channel ch, std::uint8_t controller, std::uint8_t value) noexcept;
    static MidiMessage program_change(core::Channel ch, std::uint8_t program) noexcept;

    static MidiMessage clock() noexcept;
    static MidiMessage start() noexcept;
    static MidiMessage cont() noexcept;  // `continue` is a keyword
    static MidiMessage stop() noexcept;

    bool operator==(const MidiMessage&) const noexcept = default;
};

}  // namespace turdus::midi
