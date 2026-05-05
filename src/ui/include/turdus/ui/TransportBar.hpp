#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace turdus::app {
class AppController;
}

namespace turdus::ui {

// Top-of-window transport: play/stop, tempo spin, position display, clock toggle.
// Polls the AppController via a juce::Timer at 30 Hz to keep the position label
// in sync with the engine.
class TransportBar : public juce::Component, private juce::Timer {
public:
    explicit TransportBar(app::AppController& controller);
    ~TransportBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void refresh_from_controller();

    app::AppController& controller_;

    juce::TextButton play_button_{"Play"};
    juce::TextButton stop_button_{"Stop"};
    juce::Slider tempo_slider_{juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft};
    juce::Label tempo_label_{"tempo_lbl", "BPM"};
    juce::ToggleButton clock_button_{"MIDI Clock"};
    juce::Label position_label_{"position_lbl", "0.0 beats"};
    juce::Label time_signature_label_{"ts_lbl", "4/4"};
};

}  // namespace turdus::ui
