#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <turdus/midi/FakeMidiPort.hpp>
#include <turdus/midi/MidiBackend.hpp>

namespace turdus::midi {

// In-memory MidiBackend with a configurable list of "available" port names. Hands out
// FakeMidiPort instances for any requested name (no validation against the configured
// list, since tests may want to construct ports for arbitrary names).
class FakeMidiBackend : public MidiBackend {
public:
    void set_available_ports(std::vector<std::string> ports) noexcept {
        ports_ = std::move(ports);
    }

    std::vector<std::string> list_output_ports() override { return ports_; }

    std::unique_ptr<MidiPort> create_output_port(const std::string& name) override {
        return std::make_unique<FakeMidiPort>(name);
    }

private:
    std::vector<std::string> ports_;
};

}  // namespace turdus::midi
