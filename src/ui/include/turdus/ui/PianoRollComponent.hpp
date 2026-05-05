#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <turdus/core/Channel.hpp>
#include <turdus/core/Tick.hpp>

namespace turdus::model {
class Pattern;
}

namespace turdus::ui {

// Read-only piano roll renderer. Draws (left to right):
//   [pitch axis]  [time grid + notes + playhead]  with [velocity lane] beneath.
// Scrolling is handled by an enclosing juce::Viewport — this component sizes
// itself to the full content extent.
//
// Phase 7 scope: rendering only. Editing (drag, draw, erase) lands in Phase 8.
class PianoRollComponent : public juce::Component {
public:
    static constexpr int kMinPitch = 0;
    static constexpr int kMaxPitch = 127;
    static constexpr int kPitchAxisWidth = 56;
    static constexpr int kTimeAxisHeight = 24;
    static constexpr int kVelocityLaneHeight = 80;

    static constexpr double kMinPixelsPerBeat = 16.0;
    static constexpr double kMaxPixelsPerBeat = 320.0;
    static constexpr double kMinPixelsPerPitch = 4.0;
    static constexpr double kMaxPixelsPerPitch = 24.0;

    PianoRollComponent();

    // The pattern is observed by reference; the caller must keep it alive while
    // it's set. Pass nullptr to clear.
    void set_pattern(const model::Pattern* pattern, core::Channel default_channel);
    const model::Pattern* pattern() const noexcept { return pattern_; }

    void set_playhead(core::Tick absolute_position);

    void set_horizontal_zoom(double pixels_per_beat);
    void set_vertical_zoom(double pixels_per_pitch);

    double horizontal_zoom() const noexcept { return pixels_per_beat_; }
    double vertical_zoom() const noexcept { return pixels_per_pitch_; }

    // juce::Component
    void paint(juce::Graphics& g) override;
    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& w) override;

private:
    const model::Pattern* pattern_{nullptr};
    core::Channel channel_;
    core::Tick playhead_{0};

    double pixels_per_beat_{80.0};
    double pixels_per_pitch_{10.0};

    void recompute_size();

    int total_grid_width() const;
    int total_grid_height() const;
    int total_height() const;
    int total_width() const;

    int pitch_to_y(int pitch_value) const;
    double tick_to_x(core::Tick tick) const;

    void draw_pitch_axis(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_time_axis(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_grid(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_notes(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_velocity_lane(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_playhead(juce::Graphics& g, juce::Rectangle<int> area) const;

    static bool is_black_key(int pitch_value) noexcept;
    static juce::String pitch_name(int pitch_value);
};

}  // namespace turdus::ui
