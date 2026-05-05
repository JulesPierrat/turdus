#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include <turdus/core/Tick.hpp>
#include <turdus/model/Pattern.hpp>
#include <turdus/model/Track.hpp>

namespace turdus::app {
class AppController;
}

namespace turdus::ui {

// Phase 9 v0: read-only song view. Tracks are rows, time is columns, pattern
// placements are colored blocks. Double-click on a block invokes the navigation
// callback so the host can switch the piano roll to that pattern. No
// click-to-place / drag-to-extend yet — those land in Phase 9.5.
class SongViewComponent : public juce::Component, private juce::Timer {
public:
    using NavigateCallback =
        std::function<void(model::TrackId, model::PatternId)>;

    static constexpr int kHeaderWidth = 140;
    static constexpr int kTimeAxisHeight = 22;
    static constexpr int kRowHeight = 36;
    static constexpr double kPixelsPerBeat = 30.0;

    explicit SongViewComponent(app::AppController& controller);
    ~SongViewComponent() override;

    void set_navigate_callback(NavigateCallback cb);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    void timerCallback() override;

    void draw_header(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_time_axis(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_track_rows(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_loop_region(juce::Graphics& g, juce::Rectangle<int> area) const;
    void draw_playhead(juce::Graphics& g, juce::Rectangle<int> area) const;

    int track_index_at_y(int y) const;
    core::Tick tick_at_x(int x) const;
    double tick_to_x(core::Tick t) const;

    static juce::Colour pattern_colour(model::PatternId id);

    app::AppController& controller_;
    NavigateCallback nav_;
};

}  // namespace turdus::ui
