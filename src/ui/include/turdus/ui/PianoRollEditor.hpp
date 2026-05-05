#pragma once

#include <cstddef>

#include <juce_gui_basics/juce_gui_basics.h>

#include <turdus/model/Pattern.hpp>
#include <turdus/model/Track.hpp>
#include <turdus/ui/PianoRollComponent.hpp>
#include <turdus/ui/PianoRollListener.hpp>
#include <turdus/ui/SnapResolution.hpp>

namespace turdus::app {
class AppController;
}

namespace turdus::ui {

// Wraps PianoRollComponent in a Viewport, with a toolbar above carrying the
// pattern picker, tool buttons and snap selector. Polls the AppController on a
// timer to keep the playhead and pattern picker in sync with the project /
// engine state.
class PianoRollEditor : public juce::Component,
                        public PianoRollListener,
                        public juce::KeyListener,
                        private juce::Timer {
public:
    explicit PianoRollEditor(app::AppController& controller);
    ~PianoRollEditor() override;

    // Programmatically point the editor at a specific (track, pattern). Used by
    // the song view's double-click navigation. Silently no-ops if the pair is
    // not currently in the project.
    void select_pattern(model::TrackId track_id, model::PatternId pattern_id);

    void paint(juce::Graphics& g) override;
    void resized() override;

    // PianoRollListener
    void roll_add_note(model::Note note) override;
    void roll_remove_note(model::NoteId id) override;
    void roll_move_note(model::NoteId id, core::Tick new_start,
                        core::Pitch new_pitch) override;
    void roll_resize_note(model::NoteId id, core::Tick new_length) override;

    // Bring the single-arg Component::keyPressed back into scope so the two-arg
    // KeyListener override below doesn't hide it (we only ever use the two-arg
    // one because we attach as a KeyListener to the inner PianoRollComponent).
    using juce::Component::keyPressed;

    // juce::KeyListener
    bool keyPressed(const juce::KeyPress& key,
                    juce::Component* originating_component) override;

private:
    void timerCallback() override;

    void rebuild_pattern_picker();
    void apply_picker_selection();
    void apply_snap_selection();
    void apply_tool_selection(PianoRollComponent::Tool tool);

    static int encode_id(std::size_t track_index, std::size_t pattern_index) noexcept;
    static std::pair<std::size_t, std::size_t> decode_id(int id) noexcept;

    app::AppController& controller_;

    juce::Label picker_label_{"pattern_picker_lbl", "Pattern"};
    juce::ComboBox pattern_picker_;

    juce::Label tool_label_{"tool_lbl", "Tool"};
    juce::TextButton select_button_{"Select"};
    juce::TextButton draw_button_{"Draw"};
    juce::TextButton erase_button_{"Erase"};

    juce::Label snap_label_{"snap_lbl", "Snap"};
    juce::ComboBox snap_combo_;

    juce::Viewport viewport_;
    PianoRollComponent piano_roll_;

    int current_combo_id_{0};
    std::size_t last_seen_project_signature_{0};

    // Track + pattern currently bound to the piano roll. {} when nothing is
    // displayed (empty project).
    model::TrackId active_track_id_;
    model::PatternId active_pattern_id_;
};

}  // namespace turdus::ui
