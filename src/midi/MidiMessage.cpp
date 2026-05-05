#include <turdus/midi/MidiMessage.hpp>

namespace turdus::midi {

namespace {
constexpr std::uint8_t low7(std::uint8_t v) noexcept {
    return static_cast<std::uint8_t>(v & 0x7F);
}
}  // namespace

MidiMessage MidiMessage::note_on(core::Channel ch, core::Pitch p, core::Velocity v) noexcept {
    return MidiMessage{
        static_cast<std::uint8_t>(kStatusNoteOn | ch.value()),
        p.value(),
        v.value(),
        3,
    };
}

MidiMessage MidiMessage::note_off(core::Channel ch, core::Pitch p, core::Velocity v) noexcept {
    return MidiMessage{
        static_cast<std::uint8_t>(kStatusNoteOff | ch.value()),
        p.value(),
        v.value(),
        3,
    };
}

MidiMessage MidiMessage::cc(core::Channel ch, std::uint8_t controller, std::uint8_t value) noexcept {
    return MidiMessage{
        static_cast<std::uint8_t>(kStatusCC | ch.value()),
        low7(controller),
        low7(value),
        3,
    };
}

MidiMessage MidiMessage::program_change(core::Channel ch, std::uint8_t program) noexcept {
    return MidiMessage{
        static_cast<std::uint8_t>(kStatusProgramChange | ch.value()),
        low7(program),
        0,
        2,
    };
}

MidiMessage MidiMessage::clock() noexcept    { return MidiMessage{kStatusClock,    0, 0, 1}; }
MidiMessage MidiMessage::start() noexcept    { return MidiMessage{kStatusStart,    0, 0, 1}; }
MidiMessage MidiMessage::cont() noexcept     { return MidiMessage{kStatusContinue, 0, 0, 1}; }
MidiMessage MidiMessage::stop() noexcept     { return MidiMessage{kStatusStop,     0, 0, 1}; }

}  // namespace turdus::midi
