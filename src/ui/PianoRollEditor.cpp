#include <turdus/ui/PianoRollEditor.hpp>

#include <turdus/app/AppController.hpp>
#include <turdus/model/Pattern.hpp>
#include <turdus/model/Project.hpp>
#include <turdus/model/Track.hpp>

namespace turdus::ui {

namespace {

std::size_t project_signature(const model::Project& p) {
    std::size_t h = 1469598103934665603ULL;
    auto mix = [&](std::size_t v) {
        h ^= v;
        h *= 1099511628211ULL;
    };
    mix(p.tracks().size());
    for (const auto& te : p.tracks()) {
        mix(te.id.raw());
        mix(te.track.patterns().size());
        for (const auto& pe : te.track.patterns()) {
            mix(pe.id.raw());
        }
    }
    return h;
}

// Snap combo IDs (offset from 1, since 0 is reserved by JUCE).
constexpr int kSnapQuarter = 1;
constexpr int kSnapEighth = 2;
constexpr int kSnapSixteenth = 3;
constexpr int kSnapThirtySecond = 4;

SnapResolution snap_from_combo_id(int id) noexcept {
    switch (id) {
        case kSnapQuarter:      return SnapResolution::QuarterNote;
        case kSnapEighth:       return SnapResolution::EighthNote;
        case kSnapThirtySecond: return SnapResolution::ThirtySecondNote;
        default:                return SnapResolution::SixteenthNote;
    }
}

int combo_id_from_snap(SnapResolution s) noexcept {
    switch (s) {
        case SnapResolution::QuarterNote:      return kSnapQuarter;
        case SnapResolution::EighthNote:       return kSnapEighth;
        case SnapResolution::ThirtySecondNote: return kSnapThirtySecond;
        case SnapResolution::SixteenthNote:    return kSnapSixteenth;
    }
    return kSnapSixteenth;
}

}  // namespace

int PianoRollEditor::encode_id(std::size_t track_index,
                               std::size_t pattern_index) noexcept {
    return static_cast<int>((track_index + 1) * 10000 + pattern_index + 1);
}

std::pair<std::size_t, std::size_t> PianoRollEditor::decode_id(int id) noexcept {
    const auto track = static_cast<std::size_t>(id / 10000) - 1;
    const auto pattern = static_cast<std::size_t>(id % 10000) - 1;
    return {track, pattern};
}

PianoRollEditor::PianoRollEditor(app::AppController& controller)
    : controller_(controller) {
    addAndMakeVisible(picker_label_);
    addAndMakeVisible(pattern_picker_);
    addAndMakeVisible(tool_label_);
    addAndMakeVisible(select_button_);
    addAndMakeVisible(draw_button_);
    addAndMakeVisible(erase_button_);
    addAndMakeVisible(snap_label_);
    addAndMakeVisible(snap_combo_);
    addAndMakeVisible(viewport_);

    picker_label_.setJustificationType(juce::Justification::centredLeft);
    tool_label_.setJustificationType(juce::Justification::centredLeft);
    snap_label_.setJustificationType(juce::Justification::centredLeft);

    select_button_.setRadioGroupId(1);
    draw_button_.setRadioGroupId(1);
    erase_button_.setRadioGroupId(1);
    select_button_.setClickingTogglesState(true);
    draw_button_.setClickingTogglesState(true);
    erase_button_.setClickingTogglesState(true);
    select_button_.setToggleState(true, juce::dontSendNotification);
    select_button_.onClick = [this] {
        apply_tool_selection(PianoRollComponent::Tool::Select);
    };
    draw_button_.onClick = [this] {
        apply_tool_selection(PianoRollComponent::Tool::Draw);
    };
    erase_button_.onClick = [this] {
        apply_tool_selection(PianoRollComponent::Tool::Erase);
    };

    snap_combo_.addItem("1/4", kSnapQuarter);
    snap_combo_.addItem("1/8", kSnapEighth);
    snap_combo_.addItem("1/16", kSnapSixteenth);
    snap_combo_.addItem("1/32", kSnapThirtySecond);
    snap_combo_.setSelectedId(combo_id_from_snap(piano_roll_.snap()),
                              juce::dontSendNotification);
    snap_combo_.onChange = [this] { apply_snap_selection(); };

    pattern_picker_.onChange = [this] { apply_picker_selection(); };

    viewport_.setViewedComponent(&piano_roll_, false);
    viewport_.setScrollBarsShown(true, true);
    piano_roll_.set_listener(this);
    piano_roll_.addKeyListener(this);

    rebuild_pattern_picker();
    startTimerHz(30);
}

PianoRollEditor::~PianoRollEditor() {
    stopTimer();
    piano_roll_.removeKeyListener(this);
    piano_roll_.set_listener(nullptr);
}

void PianoRollEditor::select_pattern(model::TrackId track_id,
                                     model::PatternId pattern_id) {
    const auto& project = controller_.project();
    for (std::size_t ti = 0; ti < project.tracks().size(); ++ti) {
        const auto& te = project.tracks()[ti];
        if (te.id != track_id) {
            continue;
        }
        for (std::size_t pi = 0; pi < te.track.patterns().size(); ++pi) {
            if (te.track.patterns()[pi].id == pattern_id) {
                pattern_picker_.setSelectedId(encode_id(ti, pi),
                                              juce::sendNotification);
                return;
            }
        }
    }
}

void PianoRollEditor::paint(juce::Graphics& g) {
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void PianoRollEditor::resized() {
    auto area = getLocalBounds();
    auto top = area.removeFromTop(32).reduced(6, 4);

    picker_label_.setBounds(top.removeFromLeft(60));
    pattern_picker_.setBounds(top.removeFromLeft(220));
    top.removeFromLeft(12);

    tool_label_.setBounds(top.removeFromLeft(40));
    select_button_.setBounds(top.removeFromLeft(70));
    draw_button_.setBounds(top.removeFromLeft(60));
    erase_button_.setBounds(top.removeFromLeft(60));
    top.removeFromLeft(12);

    snap_label_.setBounds(top.removeFromLeft(40));
    snap_combo_.setBounds(top.removeFromLeft(80));

    viewport_.setBounds(area);
}

void PianoRollEditor::timerCallback() {
    const auto sig = project_signature(controller_.project());
    if (sig != last_seen_project_signature_) {
        last_seen_project_signature_ = sig;
        rebuild_pattern_picker();
    }
    piano_roll_.set_playhead(controller_.position());
}

void PianoRollEditor::rebuild_pattern_picker() {
    pattern_picker_.clear(juce::dontSendNotification);
    const auto& project = controller_.project();

    int first_id = 0;
    int matched_id = 0;

    for (std::size_t ti = 0; ti < project.tracks().size(); ++ti) {
        const auto& track = project.tracks()[ti].track;
        for (std::size_t pi = 0; pi < track.patterns().size(); ++pi) {
            const auto& pattern = track.patterns()[pi].pattern;
            const auto id = encode_id(ti, pi);
            const auto label = juce::String(track.name().empty() ? "(track)" : track.name())
                               + " / "
                               + juce::String(pattern.name().empty() ? "(pattern)" : pattern.name());
            pattern_picker_.addItem(label, id);
            if (first_id == 0) {
                first_id = id;
            }
            if (id == current_combo_id_) {
                matched_id = id;
            }
        }
    }

    if (pattern_picker_.getNumItems() == 0) {
        pattern_picker_.setTextWhenNoChoicesAvailable("(no patterns)");
        current_combo_id_ = 0;
        active_track_id_ = {};
        active_pattern_id_ = {};
        piano_roll_.set_pattern(nullptr, core::Channel{0});
        return;
    }

    const int desired_id = matched_id != 0 ? matched_id : first_id;
    pattern_picker_.setSelectedId(desired_id, juce::dontSendNotification);
    current_combo_id_ = desired_id;
    apply_picker_selection();
}

void PianoRollEditor::apply_picker_selection() {
    const auto id = pattern_picker_.getSelectedId();
    if (id == 0) {
        piano_roll_.set_pattern(nullptr, core::Channel{0});
        current_combo_id_ = 0;
        active_track_id_ = {};
        active_pattern_id_ = {};
        return;
    }
    current_combo_id_ = id;

    const auto [ti, pi] = decode_id(id);
    const auto& project = controller_.project();
    if (ti >= project.tracks().size()) {
        return;
    }
    const auto& track_entry = project.tracks()[ti];
    if (pi >= track_entry.track.patterns().size()) {
        return;
    }
    const auto& pattern_entry = track_entry.track.patterns()[pi];
    active_track_id_ = track_entry.id;
    active_pattern_id_ = pattern_entry.id;
    piano_roll_.set_pattern(&pattern_entry.pattern, track_entry.track.channel());
}

void PianoRollEditor::apply_snap_selection() {
    piano_roll_.set_snap(snap_from_combo_id(snap_combo_.getSelectedId()));
}

void PianoRollEditor::apply_tool_selection(PianoRollComponent::Tool tool) {
    piano_roll_.set_tool(tool);
}

// ----- Listener (UI gestures → AppController) -----------------------------------

void PianoRollEditor::roll_add_note(model::Note note) {
    if (!active_track_id_.is_valid() || !active_pattern_id_.is_valid()) {
        return;
    }
    controller_.add_note(active_track_id_, active_pattern_id_, note);
    // Re-bind: the underlying Pattern reference may have moved if its vector
    // reallocated. Rebuild the cached pointer.
    apply_picker_selection();
}

void PianoRollEditor::roll_remove_note(model::NoteId id) {
    if (!active_track_id_.is_valid() || !active_pattern_id_.is_valid()) {
        return;
    }
    controller_.remove_note(active_track_id_, active_pattern_id_, id);
    apply_picker_selection();
}

void PianoRollEditor::roll_move_note(model::NoteId id, core::Tick new_start,
                                     core::Pitch new_pitch) {
    if (!active_track_id_.is_valid() || !active_pattern_id_.is_valid()) {
        return;
    }
    controller_.move_note(active_track_id_, active_pattern_id_, id, new_start, new_pitch);
    apply_picker_selection();
}

void PianoRollEditor::roll_resize_note(model::NoteId id, core::Tick new_length) {
    if (!active_track_id_.is_valid() || !active_pattern_id_.is_valid()) {
        return;
    }
    controller_.resize_note(active_track_id_, active_pattern_id_, id, new_length);
    apply_picker_selection();
}

// ----- Keyboard shortcuts -------------------------------------------------------

bool PianoRollEditor::keyPressed(const juce::KeyPress& key,
                                 juce::Component* /*originating*/) {
    if (key == juce::KeyPress('1')) {
        select_button_.setToggleState(true, juce::sendNotification);
        return true;
    }
    if (key == juce::KeyPress('2')) {
        draw_button_.setToggleState(true, juce::sendNotification);
        return true;
    }
    if (key == juce::KeyPress('3')) {
        erase_button_.setToggleState(true, juce::sendNotification);
        return true;
    }
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
        piano_roll_.delete_selected();
        return true;
    }
    return false;
}

}  // namespace turdus::ui
