#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <turdus/core/Channel.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/model/Note.hpp>
#include <turdus/model/Pattern.hpp>
#include <turdus/ui/PianoRollListener.hpp>
#include <turdus/ui/SnapResolution.hpp>

namespace turdus::ui {

// Editable piano roll renderer. Draws (left to right):
//   [pitch axis]  [time grid + notes + playhead]  with [velocity lane] beneath.
// Scrolling is handled by an enclosing juce::Viewport — this component sizes
// itself to the full content extent.
//
// Phase 7: rendering. Phase 8 added Select / Draw / Erase tools.
class PianoRollComponent : public juce::Component {
public:
    enum class Tool { Select, Draw, Erase };

    static constexpr int kMinPitch = 0;
    static constexpr int kMaxPitch = 127;
    static constexpr int kPitchAxisWidth = 56;
    static constexpr int kTimeAxisHeight = 24;
    static constexpr int kVelocityLaneHeight = 80;

    static constexpr double kMinPixelsPerBeat = 16.0;
    static constexpr double kMaxPixelsPerBeat = 320.0;
    static constexpr double kMinPixelsPerPitch = 4.0;
    static constexpr double kMaxPixelsPerPitch = 24.0;

    static constexpr int kResizeEdgePx = 6;
    static constexpr int kDrawDragThresholdPx = 3;

    PianoRollComponent();

    // The pattern is observed by reference; the caller must keep it alive while
    // it's set. Pass nullptr to clear.
    void set_pattern(const model::Pattern* pattern, core::Channel default_channel);
    const model::Pattern* pattern() const noexcept { return pattern_; }

    void set_listener(PianoRollListener* listener) noexcept { listener_ = listener; }

    void set_tool(Tool tool) noexcept;
    Tool tool() const noexcept { return tool_; }

    void set_snap(SnapResolution snap) noexcept { snap_ = snap; }
    SnapResolution snap() const noexcept { return snap_; }

    // Selection — Phase 8 v0 supports a single selected note at a time.
    model::NoteId selected_note() const noexcept { return selected_; }
    void clear_selection() noexcept;

    // Removes the currently selected note via the listener. No-op if none.
    void delete_selected();

    void set_playhead(core::Tick absolute_position);

    void set_horizontal_zoom(double pixels_per_beat);
    void set_vertical_zoom(double pixels_per_pitch);

    double horizontal_zoom() const noexcept { return pixels_per_beat_; }
    double vertical_zoom() const noexcept { return pixels_per_pitch_; }

    // juce::Component
    void paint(juce::Graphics& g) override;
    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& w) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    enum class HitWhere { Empty, NoteBody, NoteRightEdge };
    struct HitTest {
        HitWhere where{HitWhere::Empty};
        model::NoteId note_id;
        model::Note note{};  // copy of hit note when relevant
    };

    enum class DragMode { None, CreatingNote, MovingNote, ResizingNote };
    struct DragState {
        DragMode mode{DragMode::None};
        juce::Point<int> origin;
        // Creating: new note's pitch + start; ghost end follows mouse.
        // Moving: original note + delta from mouse origin.
        // Resizing: original note + ghost length.
        model::NoteId target;
        model::Note original{};
        model::Note ghost{};
    };

    const model::Pattern* pattern_{nullptr};
    PianoRollListener* listener_{nullptr};
    core::Channel channel_;
    core::Tick playhead_{0};

    Tool tool_{Tool::Select};
    SnapResolution snap_{SnapResolution::SixteenthNote};
    model::NoteId selected_;

    DragState drag_;

    double pixels_per_beat_{80.0};
    double pixels_per_pitch_{10.0};

    void recompute_size();

    int total_grid_width() const;
    int total_grid_height() const;
    int total_height() const;
    int total_width() const;

    int pitch_to_y(int pitch_value) const;
    double tick_to_x(core::Tick tick) const;
    int y_to_pitch(int y) const;
    core::Tick x_to_tick(int x) const;

    HitTest hit_test(juce::Point<int> p) const;

    void draw_pitch_axis(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_time_axis(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_grid(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_notes(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_velocity_lane(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_playhead(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_ghost(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_note_rect(juce::Graphics& g, const model::Note& note,
                        bool selected, bool ghost) const;

    static bool is_black_key(int pitch_value) noexcept;
    static juce::String pitch_name(int pitch_value);
};

}  // namespace turdus::ui
