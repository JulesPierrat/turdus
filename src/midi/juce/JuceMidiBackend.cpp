#include <turdus/midi/JuceMidiBackend.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <juce_audio_devices/juce_audio_devices.h>

#include <turdus/core/Tick.hpp>
#include <turdus/midi/MidiMessage.hpp>
#include <turdus/midi/MidiPort.hpp>

namespace turdus::midi {

namespace {

class JuceMidiPort : public MidiPort {
public:
    explicit JuceMidiPort(std::string name) : name_(std::move(name)) {}

    const std::string& name() const override { return name_; }

    bool open() override {
        const auto devices = juce::MidiOutput::getAvailableDevices();
        for (const auto& info : devices) {
            if (info.name.toStdString() == name_) {
                output_ = juce::MidiOutput::openDevice(info.identifier);
                return output_ != nullptr;
            }
        }
        return false;
    }

    void close() override { output_.reset(); }
    bool is_open() const override { return output_ != nullptr; }

    void send(const MidiMessage& msg, core::Tick /*deadline*/) override {
        if (!output_) {
            return;
        }
        juce::MidiMessage juce_msg;
        switch (msg.length) {
            case 1: juce_msg = juce::MidiMessage(msg.status); break;
            case 2: juce_msg = juce::MidiMessage(msg.status, msg.data1); break;
            case 3: juce_msg = juce::MidiMessage(msg.status, msg.data1, msg.data2); break;
            default: return;
        }
        // Phase 3: send-on-due. Phase 4 will route timestamped delivery.
        output_->sendMessageNow(juce_msg);
    }

private:
    std::string name_;
    std::unique_ptr<juce::MidiOutput> output_;
};

class JuceMidiBackend : public MidiBackend {
public:
    std::vector<std::string> list_output_ports() override {
        std::vector<std::string> result;
        for (const auto& info : juce::MidiOutput::getAvailableDevices()) {
            result.push_back(info.name.toStdString());
        }
        return result;
    }

    std::unique_ptr<MidiPort> create_output_port(const std::string& name) override {
        return std::make_unique<JuceMidiPort>(name);
    }
};

}  // namespace

std::unique_ptr<MidiBackend> make_juce_backend() {
    return std::make_unique<JuceMidiBackend>();
}

}  // namespace turdus::midi
