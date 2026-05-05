#include <turdus/ui/PianoRollComponent.hpp>

#include <algorithm>
#include <array>
#include <cmath>

#include <turdus/core/Ppq.hpp>
#include <turdus/model/Note.hpp>
#include <turdus/model/Pattern.hpp>

namespace turdus::ui {

namespace {

constexpr std::array<const char*, 12> kPitchClasses{"C",  "C#", "D",  "D#",
                                                    "E",  "F",  "F#", "G",
                                                    "G#", "A",  "A#", "B"};

// Velocity → hue mapping: low vel → cool, high vel → warm. Saturated, mid lightness.
juce::Colour velocity_to_colour(int velocity) {
    const float t = static_cast<float>(std::clamp(velocity, 0, 127)) / 127.0f;
    return juce::Colour::fromHSV(0.6f - 0.6f * t, 0.7f, 0.85f, 0.95f);
}

}  // namespace

PianoRollComponent::PianoRollComponent() {
    setOpaque(true);
    recompute_size();
}

void PianoRollComponent::set_pattern(const model::Pattern* pattern,
                                     core::Channel default_channel) {
    pattern_ = pattern;
    channel_ = default_channel;
    playhead_ = core::Tick{0};
    recompute_size();
    repaint();
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
    // Wheel = horizontal zoom; Shift+wheel = vertical zoom. Pan is left to the
    // enclosing Viewport's scrollbars.
    const double factor = std::pow(1.2, w.deltaY * 4.0);
    if (e.mods.isShiftDown()) {
        set_vertical_zoom(pixels_per_pitch_ * factor);
    } else if (e.mods.isCtrlDown() || e.mods.isCommandDown()) {
        set_horizontal_zoom(pixels_per_beat_ * factor);
    } else {
        // Default to horizontal zoom — most common operation.
        set_horizontal_zoom(pixels_per_beat_ * factor);
    }
}

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

int PianoRollComponent::total_grid_width() const {
    if (pattern_ == nullptr) {
        return static_cast<int>(8.0 * pixels_per_beat_);  // empty default: 8 beats
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
    // Highest pitch at the top: y(127) = kTimeAxisHeight.
    const int rows_from_top = kMaxPitch - pitch_value;
    return kTimeAxisHeight + static_cast<int>(rows_from_top * pixels_per_pitch_);
}

double PianoRollComponent::tick_to_x(core::Tick tick) const {
    return kPitchAxisWidth
           + core::ticks_to_beats(tick).value() * pixels_per_beat_;
}

bool PianoRollComponent::is_black_key(int pitch_value) noexcept {
    const int pc = ((pitch_value % 12) + 12) % 12;
    return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
}

juce::String PianoRollComponent::pitch_name(int pitch_value) {
    const int pc = ((pitch_value % 12) + 12) % 12;
    const int octave = pitch_value / 12 - 1;  // MIDI: 60 = C4
    return juce::String(kPitchClasses[static_cast<std::size_t>(pc)])
           + juce::String(octave);
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void PianoRollComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
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
    draw_velocity_lane(g, velocity_area);
    draw_playhead(g, grid_area);
    draw_pitch_axis(g, pitch_axis_area);
    draw_time_axis(g, time_axis_area);

    // Border at the very top-left corner so the axes look anchored.
    g.setColour(juce::Colour::fromRGB(40, 40, 48));
    g.fillRect(0, 0, kPitchAxisWidth, kTimeAxisHeight);

    juce::ignoreUnused(bounds);
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

        // Octave label every C
        if (p % 12 == 0) {
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.setFont(juce::Font(juce::FontOptions(11.0f)));
            g.drawText(pitch_name(p), row.reduced(4, 0), juce::Justification::centredLeft);
        }
    }

    // Right border to separate from grid
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

    // Horizontal pitch rows
    for (int p = kMinPitch; p <= kMaxPitch; ++p) {
        const int y = pitch_to_y(p);
        if (is_black_key(p)) {
            const int height = std::max(1, static_cast<int>(pixels_per_pitch_));
            g.setColour(juce::Colour::fromRGB(28, 28, 34));
            g.fillRect(area.getX(), y, area.getWidth(), height);
        }
        if (p % 12 == 0) {  // octave separator
            g.setColour(juce::Colour::fromRGB(50, 50, 60));
            g.drawHorizontalLine(y, static_cast<float>(area.getX()),
                                 static_cast<float>(area.getRight()));
        }
    }

    // Vertical beat / bar lines
    const double total_beats =
        pattern_ != nullptr
            ? core::ticks_to_beats(pattern_->length()).value()
            : 8.0;
    for (int beat = 0; beat <= static_cast<int>(total_beats); ++beat) {
        const double x = kPitchAxisWidth + beat * pixels_per_beat_;
        const bool is_bar = beat % 4 == 0;
        g.setColour(is_bar
                        ? juce::Colour::fromRGB(70, 70, 84)
                        : juce::Colour::fromRGB(50, 50, 60));
        g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()),
                           static_cast<float>(area.getBottom()));
    }
}

void PianoRollComponent::draw_notes(juce::Graphics& g,
                                    juce::Rectangle<int> area) const {
    if (pattern_ == nullptr) {
        return;
    }
    juce::Graphics::ScopedSaveState saver(g);
    g.reduceClipRegion(area);

    for (const auto& entry : pattern_->notes()) {
        const auto& n = entry.note;
        const auto x_start = tick_to_x(n.start);
        const auto x_end = tick_to_x(n.end());
        const int y = pitch_to_y(n.pitch.value());
        const int h = std::max(2, static_cast<int>(pixels_per_pitch_) - 1);
        const auto rect = juce::Rectangle<float>(
            static_cast<float>(x_start), static_cast<float>(y),
            static_cast<float>(std::max(2.0, x_end - x_start)),
            static_cast<float>(h));

        const auto colour = velocity_to_colour(n.velocity.value());
        g.setColour(colour);
        g.fillRoundedRectangle(rect, 2.0f);
        g.setColour(colour.darker(0.4f));
        g.drawRoundedRectangle(rect, 2.0f, 1.0f);
    }
    juce::ignoreUnused(channel_);  // Phase 8 will use it for non-default-channel notes
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
