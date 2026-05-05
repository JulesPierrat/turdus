#pragma once

#include <cstddef>

#include <juce_gui_basics/juce_gui_basics.h>

#include <turdus/ui/PianoRollComponent.hpp>

namespace turdus::app {
class AppController;
}

namespace turdus::ui {

// Wraps PianoRollComponent in a Viewport, with a small toolbar above it carrying
// the pattern picker. Polls the AppController on a timer to keep the playhead and
// pattern picker in sync with the project / engine state.
class PianoRollEditor : public juce::Component, private juce::Timer {
public:
    explicit PianoRollEditor(app::AppController& controller);
    ~PianoRollEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    void rebuild_pattern_picker();
    void apply_picker_selection();

    // Encodes (track_index, pattern_index) into a non-zero combo id so JUCE's
    // ComboBox machinery is happy.
    static int encode_id(std::size_t track_index, std::size_t pattern_index) noexcept;
    static std::pair<std::size_t, std::size_t> decode_id(int id) noexcept;

    app::AppController& controller_;

    juce::Label picker_label_{"pattern_picker_lbl", "Pattern"};
    juce::ComboBox pattern_picker_;
    juce::Viewport viewport_;
    PianoRollComponent piano_roll_;

    int current_combo_id_{0};
    std::size_t last_seen_project_signature_{0};
};

}  // namespace turdus::ui
