#pragma once

#include <string>
#include <utility>
#include <vector>

#include <turdus/core/Tick.hpp>
#include <turdus/midi/MidiMessage.hpp>
#include <turdus/midi/MidiPort.hpp>

namespace turdus::midi {

// In-memory MidiPort that records every message it receives. Used by engine and IO
// tests to assert exact event sequences without touching a real device.
class FakeMidiPort : public MidiPort {
public:
    struct SentMessage {
        MidiMessage msg;
        core::Tick deadline;

        bool operator==(const SentMessage&) const noexcept = default;
    };

    explicit FakeMidiPort(std::string name) : name_(std::move(name)) {}

    const std::string& name() const override { return name_; }

    bool open() override {
        is_open_ = true;
        return true;
    }
    void close() override { is_open_ = false; }
    bool is_open() const override { return is_open_; }

    // Default repeated here: defaults on virtuals are resolved from the static type at
    // the call site, so callers using a FakeMidiPort directly (tests) need this default.
    void send(const MidiMessage& msg, core::Tick deadline = core::Tick{0}) override {
        if (!is_open_) {
            return;
        }
        sent_.push_back(SentMessage{msg, deadline});
    }

    const std::vector<SentMessage>& sent() const noexcept { return sent_; }
    void clear() noexcept { sent_.clear(); }

private:
    std::string name_;
    bool is_open_{false};
    std::vector<SentMessage> sent_;
};

}  // namespace turdus::midi
