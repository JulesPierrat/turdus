#include <turdus/ui/PianoRollComponent.hpp>

#include <algorithm>
#include <array>
#include <cmath>

#include <turdus/core/Ppq.hpp>

namespace turdus::ui {

namespace {

constexpr std::array<const char*, 12> kPitchClasses{"C",  "C#", "D",  "D#",
                                                    "E",  "F",  "F#", "G",
                                                    "G#", "A",  "A#", "B"};

juce::Colour velocity_to_colour(int velocity) {
    const float t = static_cast<float>(std::clamp(velocity, 0, 127)) / 127.0f;
    return juce::Colour::fromHSV(0.6f - 0.6f * t, 0.7f, 0.85f, 0.95f);
}

}  // namespace

PianoRollComponent::PianoRollComponent() {
    setOpaque(true);
    setWantsKeyboardFocus(true);
    recompute_size();
}

void PianoRollComponent::set_pattern(const model::Pattern* pattern,
                                     core::Channel default_channel) {
    pattern_ = pattern;
    channel_ = default_channel;
    playhead_ = core::Tick{0};
    selected_ = {};
    drag_ = {};
    recompute_size();
    repaint();
}

void PianoRollComponent::set_tool(Tool tool) noexcept {
    if (tool_ == tool) {
        return;
    }
    tool_ = tool;
    drag_ = {};
    if (tool != Tool::Select) {
        selected_ = {};
    }
    repaint();
}

void PianoRollComponent::clear_selection() noexcept {
    if (!selected_.is_valid()) {
        return;
    }
    selected_ = {};
    repaint();
}

void PianoRollComponent::delete_selected() {
    if (!selected_.is_valid() || listener_ == nullptr) {
        return;
    }
    listener_->roll_remove_note(selected_);
    selected_ = {};
}

void PianoRollComponent::set_playhead(core::Tick absolute_position) {
    if (absolute_position == playhead_) {
        return;
    }
    playhead_ = absolute_position;
    repaint();
}

void PianoRollComponent::set_horizontal_zoom(double pixels_per_beat) {
    const auto clamped = std::clamp(pixels_per_beat, kMinPixelsPerBeat, kMaxPixelsPerBeat);
    if (std::abs(clamped - pixels_per_beat_) < 0.001) {
        return;
    }
    pixels_per_beat_ = clamped;
    recompute_size();
    repaint();
}

void PianoRollComponent::set_vertical_zoom(double pixels_per_pitch) {
    const auto clamped = std::clamp(pixels_per_pitch, kMinPixelsPerPitch, kMaxPixelsPerPitch);
    if (std::abs(clamped - pixels_per_pitch_) < 0.001) {
        return;
    }
    pixels_per_pitch_ = clamped;
    recompute_size();
    repaint();
}

void PianoRollComponent::mouseWheelMove(const juce::MouseEvent& e,
                                        const juce::MouseWheelDetails& w) {
    const double factor = std::pow(1.2, w.deltaY * 4.0);
    if (e.mods.isShiftDown()) {
        set_vertical_zoom(pixels_per_pitch_ * factor);
    } else if (e.mods.isCtrlDown() || e.mods.isCommandDown()) {
        set_horizontal_zoom(pixels_per_beat_ * factor);
    } else {
        set_horizontal_zoom(pixels_per_beat_ * factor);
    }
}

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

int PianoRollComponent::total_grid_width() const {
    if (pattern_ == nullptr) {
        return static_cast<int>(8.0 * pixels_per_beat_);
    }
    const auto beats = core::ticks_to_beats(pattern_->length()).value();
    return std::max(static_cast<int>(beats * pixels_per_beat_), 1);
}

int PianoRollComponent::total_grid_height() const {
    return static_cast<int>((kMaxPitch - kMinPitch + 1) * pixels_per_pitch_);
}

int PianoRollComponent::total_height() const {
    return kTimeAxisHeight + total_grid_height() + kVelocityLaneHeight;
}

int PianoRollComponent::total_width() const {
    return kPitchAxisWidth + total_grid_width();
}

void PianoRollComponent::recompute_size() {
    setSize(total_width(), total_height());
}

int PianoRollComponent::pitch_to_y(int pitch_value) const {
    const int rows_from_top = kMaxPitch - pitch_value;
    return kTimeAxisHeight + static_cast<int>(rows_from_top * pixels_per_pitch_);
}

double PianoRollComponent::tick_to_x(core::Tick tick) const {
    return kPitchAxisWidth
           + core::ticks_to_beats(tick).value() * pixels_per_beat_;
}

int PianoRollComponent::y_to_pitch(int y) const {
    const int local_y = y - kTimeAxisHeight;
    if (local_y < 0) {
        return kMaxPitch;
    }
    int rows = static_cast<int>(local_y / pixels_per_pitch_);
    return std::clamp(kMaxPitch - rows, kMinPitch, kMaxPitch);
}

core::Tick PianoRollComponent::x_to_tick(int x) const {
    if (x < kPitchAxisWidth) {
        return core::Tick{0};
    }
    const double beats = (x - kPitchAxisWidth) / pixels_per_beat_;
    return core::beats_to_ticks(core::Beats{beats});
}

bool PianoRollComponent::is_black_key(int pitch_value) noexcept {
    const int pc = ((pitch_value % 12) + 12) % 12;
    return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
}

juce::String PianoRollComponent::pitch_name(int pitch_value) {
    const int pc = ((pitch_value % 12) + 12) % 12;
    const int octave = pitch_value / 12 - 1;
    return juce::String(kPitchClasses[static_cast<std::size_t>(pc)])
           + juce::String(octave);
}

// ---------------------------------------------------------------------------
// Hit testing
// ---------------------------------------------------------------------------

PianoRollComponent::HitTest PianoRollComponent::hit_test(juce::Point<int> p) const {
    HitTest result;
    if (pattern_ == nullptr) {
        return result;
    }
    if (p.x < kPitchAxisWidth || p.y < kTimeAxisHeight
        || p.y >= kTimeAxisHeight + total_grid_height()) {
        return result;
    }

    // Iterate in reverse so later-drawn (visually on top) notes win when overlapping.
    const auto& notes = pattern_->notes();
    for (auto it = notes.rbegin(); it != notes.rend(); ++it) {
        const auto& n = it->note;
        const int y = pitch_to_y(n.pitch.value());
        const int h = std::max(2, static_cast<int>(pixels_per_pitch_) - 1);
        if (p.y < y || p.y >= y + h) {
            continue;
        }
        const auto x_start = tick_to_x(n.start);
        const auto x_end = tick_to_x(n.end());
        if (p.x < x_start || p.x >= x_end) {
            continue;
        }
        result.note_id = it->id;
        result.note = n;
        result.where = (x_end - p.x) <= kResizeEdgePx ? HitWhere::NoteRightEdge
                                                     : HitWhere::NoteBody;
        return result;
    }
    return result;
}

// ---------------------------------------------------------------------------
// Mouse handlers
// ---------------------------------------------------------------------------

void PianoRollComponent::mouseDown(const juce::MouseEvent& e) {
    grabKeyboardFocus();
    drag_ = {};
    drag_.origin = e.getPosition();

    const auto hit = hit_test(e.getPosition());

    if (tool_ == Tool::Erase) {
        if (hit.where != HitWhere::Empty && listener_ != nullptr) {
            listener_->roll_remove_note(hit.note_id);
        }
        return;
    }

    if (tool_ == Tool::Draw) {
        const auto pitch = core::Pitch{y_to_pitch(e.y)};
        const auto raw_start = x_to_tick(e.x);
        const auto start = snap_floor(raw_start, snap_);
        const auto length = core::Tick{snap_ticks(snap_)};
        drag_.mode = DragMode::CreatingNote;
        drag_.ghost = model::Note{pitch, start, length, core::Velocity{100}};
        repaint();
        return;
    }

    // Tool::Select
    if (hit.where == HitWhere::Empty) {
        clear_selection();
        return;
    }

    selected_ = hit.note_id;
    if (hit.where == HitWhere::NoteRightEdge) {
        drag_.mode = DragMode::ResizingNote;
    } else {
        drag_.mode = DragMode::MovingNote;
    }
    drag_.target = hit.note_id;
    drag_.original = hit.note;
    drag_.ghost = hit.note;
    repaint();
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& e) {
    if (drag_.mode == DragMode::None) {
        return;
    }
    const int dx = e.x - drag_.origin.x;
    const int dy = e.y - drag_.origin.y;

    switch (drag_.mode) {
        case DragMode::CreatingNote: {
            const auto raw_end = x_to_tick(e.x);
            const auto snapped_end = snap_floor(raw_end, snap_);
            auto length = snapped_end.value() + snap_ticks(snap_)
                          - drag_.ghost.start.value();
            length = std::max<std::int64_t>(snap_ticks(snap_), length);
            drag_.ghost.length = core::Tick{length};
            // Allow vertical pitch change while dragging.
            drag_.ghost.pitch = core::Pitch{y_to_pitch(e.y)};
            break;
        }
        case DragMode::MovingNote: {
            const auto delta_ticks = core::beats_to_ticks(
                core::Beats{dx / pixels_per_beat_});
            auto new_start = drag_.original.start.value() + delta_ticks.value();
            new_start = std::max<std::int64_t>(0, new_start);
            const auto snapped = snap_floor(core::Tick{new_start}, snap_);
            drag_.ghost.start = snapped;

            const int pitch_rows = static_cast<int>(std::round(dy / pixels_per_pitch_));
            const int new_pitch = std::clamp(
                drag_.original.pitch.value() - pitch_rows, kMinPitch, kMaxPitch);
            drag_.ghost.pitch = core::Pitch{new_pitch};
            break;
        }
        case DragMode::ResizingNote: {
            const auto delta_ticks = core::beats_to_ticks(
                core::Beats{dx / pixels_per_beat_});
            auto new_length = drag_.original.length.value() + delta_ticks.value();
            new_length = std::max<std::int64_t>(snap_ticks(snap_), new_length);
            // Snap the END position rather than the length, so resize aligns to grid.
            const auto end_unsnapped = drag_.original.start.value() + new_length;
            const auto end_snapped = snap_floor(core::Tick{end_unsnapped}, snap_);
            const auto snapped_length = std::max<std::int64_t>(
                snap_ticks(snap_), end_snapped.value() - drag_.original.start.value());
            drag_.ghost.length = core::Tick{snapped_length};
            break;
        }
        case DragMode::None: break;
    }
    repaint();
}

void PianoRollComponent::mouseUp(const juce::MouseEvent& /*e*/) {
    if (drag_.mode == DragMode::None || listener_ == nullptr) {
        drag_ = {};
        return;
    }

    switch (drag_.mode) {
        case DragMode::CreatingNote: {
            listener_->roll_add_note(drag_.ghost);
            break;
        }
        case DragMode::MovingNote: {
            if (drag_.ghost.start != drag_.original.start
                || drag_.ghost.pitch != drag_.original.pitch) {
                listener_->roll_move_note(drag_.target, drag_.ghost.start,
                                          drag_.ghost.pitch);
            }
            break;
        }
        case DragMode::ResizingNote: {
            if (drag_.ghost.length != drag_.original.length) {
                listener_->roll_resize_note(drag_.target, drag_.ghost.length);
            }
            break;
        }
        case DragMode::None: break;
    }
    drag_ = {};
    repaint();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void PianoRollComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromRGB(28, 28, 32));

    const auto pitch_axis_area = juce::Rectangle<int>{
        0, kTimeAxisHeight, kPitchAxisWidth, total_grid_height()};
    const auto time_axis_area = juce::Rectangle<int>{
        kPitchAxisWidth, 0, total_grid_width(), kTimeAxisHeight};
    const auto grid_area = juce::Rectangle<int>{
        kPitchAxisWidth, kTimeAxisHeight, total_grid_width(), total_grid_height()};
    const auto velocity_area = juce::Rectangle<int>{
        kPitchAxisWidth, kTimeAxisHeight + total_grid_height(),
        total_grid_width(), kVelocityLaneHeight};

    draw_grid(g, grid_area);
    draw_notes(g, grid_area);
    draw_ghost(g, grid_area);
    draw_velocity_lane(g, velocity_area);
    draw_playhead(g, grid_area);
    draw_pitch_axis(g, pitch_axis_area);
    draw_time_axis(g, time_axis_area);

    g.setColour(juce::Colour::fromRGB(40, 40, 48));
    g.fillRect(0, 0, kPitchAxisWidth, kTimeAxisHeight);
}

void PianoRollComponent::draw_pitch_axis(juce::Graphics& g,
                                         juce::Rectangle<int> area) const {
    g.setColour(juce::Colour::fromRGB(36, 36, 42));
    g.fillRect(area);

    for (int p = kMinPitch; p <= kMaxPitch; ++p) {
        const int y = pitch_to_y(p);
        const int height = std::max(1, static_cast<int>(pixels_per_pitch_));
        const auto row = juce::Rectangle<int>{area.getX(), y, area.getWidth(), height};

        if (is_black_key(p)) {
            g.setColour(juce::Colour::fromRGB(22, 22, 26));
            g.fillRect(row);
        }

        if (p % 12 == 0) {
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.setFont(juce::Font(juce::FontOptions(11.0f)));
            g.drawText(pitch_name(p), row.reduced(4, 0), juce::Justification::centredLeft);
        }
    }

    g.setColour(juce::Colour::fromRGB(60, 60, 70));
    g.drawVerticalLine(area.getRight() - 1, static_cast<float>(area.getY()),
                       static_cast<float>(area.getBottom()));
}

void PianoRollComponent::draw_time_axis(juce::Graphics& g,
                                        juce::Rectangle<int> area) const {
    g.setColour(juce::Colour::fromRGB(36, 36, 42));
    g.fillRect(area);

    const double total_beats =
        pattern_ != nullptr
            ? core::ticks_to_beats(pattern_->length()).value()
            : 8.0;

    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));

    for (int beat = 0; beat <= static_cast<int>(total_beats); ++beat) {
        const double x = kPitchAxisWidth + beat * pixels_per_beat_;
        g.drawText(juce::String(beat + 1),
                   juce::Rectangle<int>{static_cast<int>(x) + 2, area.getY(),
                                        static_cast<int>(pixels_per_beat_),
                                        area.getHeight()},
                   juce::Justification::centredLeft);
    }

    g.setColour(juce::Colour::fromRGB(60, 60, 70));
    g.drawHorizontalLine(area.getBottom() - 1, static_cast<float>(area.getX()),
                         static_cast<float>(area.getRight()));
}

void PianoRollComponent::draw_grid(juce::Graphics& g,
                                   juce::Rectangle<int> area) const {
    g.setColour(juce::Colour::fromRGB(34, 34, 40));
    g.fillRect(area);

    for (int p = kMinPitch; p <= kMaxPitch; ++p) {
        const int y = pitch_to_y(p);
        if (is_black_key(p)) {
            const int height = std::max(1, static_cast<int>(pixels_per_pitch_));
            g.setColour(juce::Colour::fromRGB(28, 28, 34));
            g.fillRect(area.getX(), y, area.getWidth(), height);
        }
        if (p % 12 == 0) {
            g.setColour(juce::Colour::fromRGB(50, 50, 60));
            g.drawHorizontalLine(y, static_cast<float>(area.getX()),
                                 static_cast<float>(area.getRight()));
        }
    }

    // Beat / sub-beat lines based on snap resolution.
    const auto snap_step = snap_ticks(snap_);
    const auto pattern_length =
        pattern_ != nullptr ? pattern_->length().value() : core::Tick::value_type{8 * 960};

    for (auto t = core::Tick::value_type{0}; t <= pattern_length; t += snap_step) {
        const double x = kPitchAxisWidth + (t / 960.0) * pixels_per_beat_;
        const bool is_beat = t % 960 == 0;
        const bool is_bar = t % (960 * 4) == 0;
        if (is_bar) {
            g.setColour(juce::Colour::fromRGB(80, 80, 96));
        } else if (is_beat) {
            g.setColour(juce::Colour::fromRGB(60, 60, 72));
        } else {
            g.setColour(juce::Colour::fromRGB(45, 45, 54));
        }
        g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()),
                           static_cast<float>(area.getBottom()));
    }
}

void PianoRollComponent::draw_note_rect(juce::Graphics& g, const model::Note& note,
                                        bool selected, bool ghost) const {
    const auto x_start = tick_to_x(note.start);
    const auto x_end = tick_to_x(note.end());
    const int y = pitch_to_y(note.pitch.value());
    const int h = std::max(2, static_cast<int>(pixels_per_pitch_) - 1);
    const auto rect = juce::Rectangle<float>(
        static_cast<float>(x_start), static_cast<float>(y),
        static_cast<float>(std::max(2.0, x_end - x_start)),
        static_cast<float>(h));

    auto colour = velocity_to_colour(note.velocity.value());
    if (ghost) {
        colour = colour.withAlpha(0.5f);
    }
    g.setColour(colour);
    g.fillRoundedRectangle(rect, 2.0f);
    g.setColour(selected ? juce::Colours::white : colour.darker(0.4f));
    g.drawRoundedRectangle(rect, 2.0f, selected ? 2.0f : 1.0f);
}

void PianoRollComponent::draw_notes(juce::Graphics& g,
                                    juce::Rectangle<int> area) const {
    if (pattern_ == nullptr) {
        return;
    }
    juce::Graphics::ScopedSaveState saver(g);
    g.reduceClipRegion(area);

    for (const auto& entry : pattern_->notes()) {
        if (drag_.mode == DragMode::MovingNote && entry.id == drag_.target) {
            continue;  // ghost will be drawn by draw_ghost
        }
        if (drag_.mode == DragMode::ResizingNote && entry.id == drag_.target) {
            continue;
        }
        const bool is_selected = entry.id == selected_;
        draw_note_rect(g, entry.note, is_selected, false);
    }
    juce::ignoreUnused(channel_);
}

void PianoRollComponent::draw_ghost(juce::Graphics& g,
                                    juce::Rectangle<int> area) const {
    if (drag_.mode == DragMode::None) {
        return;
    }
    juce::Graphics::ScopedSaveState saver(g);
    g.reduceClipRegion(area);
    draw_note_rect(g, drag_.ghost, /*selected=*/true, /*ghost=*/true);
}

void PianoRollComponent::draw_velocity_lane(juce::Graphics& g,
                                            juce::Rectangle<int> area) const {
    g.setColour(juce::Colour::fromRGB(24, 24, 28));
    g.fillRect(area);
    g.setColour(juce::Colour::fromRGB(60, 60, 70));
    g.drawHorizontalLine(area.getY(), static_cast<float>(area.getX()),
                         static_cast<float>(area.getRight()));

    if (pattern_ == nullptr) {
        return;
    }

    juce::Graphics::ScopedSaveState saver(g);
    g.reduceClipRegion(area);

    const float baseline = static_cast<float>(area.getBottom());
    const float lane_h = static_cast<float>(area.getHeight() - 4);
    constexpr int bar_width = 4;

    for (const auto& entry : pattern_->notes()) {
        const auto& n = entry.note;
        const auto x_start = tick_to_x(n.start);
        const float h = lane_h * static_cast<float>(n.velocity.value()) / 127.0f;
        const auto bar = juce::Rectangle<float>(
            static_cast<float>(x_start), baseline - h,
            static_cast<float>(bar_width), h);
        g.setColour(velocity_to_colour(n.velocity.value()));
        g.fillRect(bar);
    }
}

void PianoRollComponent::draw_playhead(juce::Graphics& g,
                                       juce::Rectangle<int> area) const {
    if (pattern_ == nullptr || pattern_->length().value() <= 0) {
        return;
    }
    const auto local =
        core::Tick{playhead_.value() % pattern_->length().value()};
    const auto x = tick_to_x(local);
    if (x < area.getX() || x > area.getRight()) {
        return;
    }
    g.setColour(juce::Colours::orangered.withAlpha(0.85f));
    g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()),
                       static_cast<float>(area.getBottom() + kVelocityLaneHeight));
}

}  // namespace turdus::ui
