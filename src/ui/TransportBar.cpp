#include <turdus/ui/TransportBar.hpp>

#include <turdus/app/AppController.hpp>
#include <turdus/core/Bpm.hpp>
#include <turdus/core/Ppq.hpp>

namespace turdus::ui {

TransportBar::TransportBar(app::AppController& controller) : controller_(controller) {
    addAndMakeVisible(play_button_);
    addAndMakeVisible(stop_button_);
    addAndMakeVisible(tempo_slider_);
    addAndMakeVisible(tempo_label_);
    addAndMakeVisible(clock_button_);
    addAndMakeVisible(position_label_);
    addAndMakeVisible(time_signature_label_);

    play_button_.onClick = [this] { controller_.play(); refresh_from_controller(); };
    stop_button_.onClick = [this] { controller_.stop(); refresh_from_controller(); };

    tempo_slider_.setRange(core::Bpm::kMin, core::Bpm::kMax, 0.1);
    tempo_slider_.setValue(controller_.tempo().value(), juce::dontSendNotification);
    tempo_slider_.setTextValueSuffix(" BPM");
    tempo_slider_.onValueChange = [this] {
        controller_.set_tempo(core::Bpm{tempo_slider_.getValue()});
    };

    clock_button_.setToggleState(controller_.clock_enabled(), juce::dontSendNotification);
    clock_button_.onClick = [this] {
        controller_.set_clock_enabled(clock_button_.getToggleState());
    };

    tempo_label_.setJustificationType(juce::Justification::centredRight);
    position_label_.setJustificationType(juce::Justification::centred);
    time_signature_label_.setJustificationType(juce::Justification::centred);

    startTimerHz(30);
    refresh_from_controller();
}

TransportBar::~TransportBar() { stopTimer(); }

void TransportBar::paint(juce::Graphics& g) {
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId).darker(0.1f));
}

void TransportBar::resized() {
    auto area = getLocalBounds().reduced(8);
    const int btn_w = 70;
    const int gap = 6;

    play_button_.setBounds(area.removeFromLeft(btn_w));
    area.removeFromLeft(gap);
    stop_button_.setBounds(area.removeFromLeft(btn_w));
    area.removeFromLeft(gap);

    tempo_label_.setBounds(area.removeFromLeft(40));
    tempo_slider_.setBounds(area.removeFromLeft(180));
    area.removeFromLeft(gap);

    clock_button_.setBounds(area.removeFromLeft(120));
    area.removeFromLeft(gap);

    time_signature_label_.setBounds(area.removeFromLeft(60));
    position_label_.setBounds(area);
}

void TransportBar::timerCallback() { refresh_from_controller(); }

void TransportBar::refresh_from_controller() {
    const auto playing = controller_.is_playing();
    play_button_.setEnabled(!playing);
    stop_button_.setEnabled(playing);

    const auto position_beats =
        core::ticks_to_beats(controller_.position()).value();
    position_label_.setText(juce::String(position_beats, 2) + " beats",
                            juce::dontSendNotification);

    const auto& ts = controller_.project().time_signature();
    time_signature_label_.setText(
        juce::String(ts.numerator()) + "/" + juce::String(ts.denominator()),
        juce::dontSendNotification);

    // Re-read tempo in case a project load changed it.
    const auto current_tempo = controller_.tempo().value();
    if (std::abs(tempo_slider_.getValue() - current_tempo) > 0.001) {
        tempo_slider_.setValue(current_tempo, juce::dontSendNotification);
    }

    clock_button_.setToggleState(controller_.clock_enabled(),
                                 juce::dontSendNotification);
}

}  // namespace turdus::ui
