#include <turdus/ui/PianoRollEditor.hpp>

#include <turdus/app/AppController.hpp>
#include <turdus/model/Pattern.hpp>
#include <turdus/model/Project.hpp>
#include <turdus/model/Track.hpp>

namespace turdus::ui {

namespace {

// Cheap "did the project's track/pattern shape change?" signature. Doesn't need to
// be cryptographically unique — we just want a collision-rare digest so we know
// when to rebuild the picker.
std::size_t project_signature(const model::Project& p) {
    std::size_t h = 1469598103934665603ULL;  // FNV-1a offset
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

}  // namespace

int PianoRollEditor::encode_id(std::size_t track_index,
                               std::size_t pattern_index) noexcept {
    // Combo IDs must be non-zero. Pack track index in the high half + 1 to keep > 0.
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
    addAndMakeVisible(viewport_);

    picker_label_.setJustificationType(juce::Justification::centredLeft);

    viewport_.setViewedComponent(&piano_roll_, false);
    viewport_.setScrollBarsShown(true, true);

    pattern_picker_.onChange = [this] { apply_picker_selection(); };

    rebuild_pattern_picker();
    startTimerHz(30);
}

PianoRollEditor::~PianoRollEditor() { stopTimer(); }

void PianoRollEditor::paint(juce::Graphics& g) {
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void PianoRollEditor::resized() {
    auto area = getLocalBounds();
    auto top = area.removeFromTop(28).reduced(6, 2);
    picker_label_.setBounds(top.removeFromLeft(60));
    pattern_picker_.setBounds(top.removeFromLeft(280));

    viewport_.setBounds(area);
}

void PianoRollEditor::timerCallback() {
    // Keep the picker in sync with project shape changes (e.g. after Open).
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
        return;
    }
    current_combo_id_ = id;

    const auto [ti, pi] = decode_id(id);
    const auto& project = controller_.project();
    if (ti >= project.tracks().size()) {
        return;
    }
    const auto& track = project.tracks()[ti].track;
    if (pi >= track.patterns().size()) {
        return;
    }
    piano_roll_.set_pattern(&track.patterns()[pi].pattern, track.channel());
}

}  // namespace turdus::ui
