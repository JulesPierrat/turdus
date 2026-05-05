#include <turdus/ui/SongViewComponent.hpp>

#include <algorithm>
#include <utility>

#include <turdus/app/AppController.hpp>
#include <turdus/core/Ppq.hpp>
#include <turdus/model/Pattern.hpp>
#include <turdus/model/Project.hpp>
#include <turdus/model/Track.hpp>

namespace turdus::ui {

SongViewComponent::SongViewComponent(app::AppController& controller)
    : controller_(controller) {
    setOpaque(true);
    startTimerHz(15);  // playhead refresh; lighter than 30 Hz since the view is dense
}

SongViewComponent::~SongViewComponent() { stopTimer(); }

void SongViewComponent::set_navigate_callback(NavigateCallback cb) {
    nav_ = std::move(cb);
}

void SongViewComponent::resized() { repaint(); }

void SongViewComponent::timerCallback() {
    // Cheap repaint — we only paint a thin playhead column, but JUCE clips it
    // for us. No-op when invisible.
    if (isVisible()) {
        repaint();
    }
}

// ---------------------------------------------------------------------------
// Coords
// ---------------------------------------------------------------------------

double SongViewComponent::tick_to_x(core::Tick t) const {
    return kHeaderWidth
           + core::ticks_to_beats(t).value() * kPixelsPerBeat;
}

core::Tick SongViewComponent::tick_at_x(int x) const {
    if (x < kHeaderWidth) {
        return core::Tick{0};
    }
    const double beats = (x - kHeaderWidth) / kPixelsPerBeat;
    return core::beats_to_ticks(core::Beats{beats});
}

int SongViewComponent::track_index_at_y(int y) const {
    if (y < kTimeAxisHeight) {
        return -1;
    }
    return (y - kTimeAxisHeight) / kRowHeight;
}

juce::Colour SongViewComponent::pattern_colour(model::PatternId id) {
    // Stable HSV-derived colour per pattern id — different patterns get
    // different hues, same pattern always gets the same colour.
    const auto raw = id.raw();
    const float hue = static_cast<float>(raw % 360) / 360.0f;
    return juce::Colour::fromHSV(hue, 0.55f, 0.78f, 1.0f);
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void SongViewComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromRGB(28, 28, 32));

    const auto bounds = getLocalBounds();
    const auto header_area =
        juce::Rectangle<int>{0, kTimeAxisHeight, kHeaderWidth, bounds.getHeight() - kTimeAxisHeight};
    const auto time_area =
        juce::Rectangle<int>{kHeaderWidth, 0, bounds.getWidth() - kHeaderWidth, kTimeAxisHeight};
    const auto rows_area =
        juce::Rectangle<int>{kHeaderWidth, kTimeAxisHeight,
                             bounds.getWidth() - kHeaderWidth,
                             bounds.getHeight() - kTimeAxisHeight};

    draw_loop_region(g, rows_area);
    draw_track_rows(g, rows_area);
    draw_playhead(g, rows_area);
    draw_header(g, header_area);
    draw_time_axis(g, time_area);

    // Top-left corner block.
    g.setColour(juce::Colour::fromRGB(40, 40, 48));
    g.fillRect(0, 0, kHeaderWidth, kTimeAxisHeight);
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawText("Song", juce::Rectangle<int>{0, 0, kHeaderWidth, kTimeAxisHeight}.reduced(6, 0),
               juce::Justification::centredLeft);
}

void SongViewComponent::draw_header(juce::Graphics& g,
                                    juce::Rectangle<int> area) const {
    g.setColour(juce::Colour::fromRGB(36, 36, 42));
    g.fillRect(area);

    const auto& tracks = controller_.project().tracks();
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        const auto y = area.getY() + static_cast<int>(i * kRowHeight);
        const auto row =
            juce::Rectangle<int>{area.getX(), y, area.getWidth(), kRowHeight};
        g.setColour((i % 2) == 0 ? juce::Colour::fromRGB(34, 34, 40)
                                 : juce::Colour::fromRGB(38, 38, 46));
        g.fillRect(row);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        const auto name = tracks[i].track.name().empty()
                              ? juce::String("(track ") + juce::String(i + 1) + ")"
                              : juce::String(tracks[i].track.name());
        g.drawText(name, row.reduced(8, 0), juce::Justification::centredLeft);
    }

    g.setColour(juce::Colour::fromRGB(60, 60, 72));
    g.drawVerticalLine(area.getRight() - 1, static_cast<float>(area.getY()),
                       static_cast<float>(area.getBottom()));
}

void SongViewComponent::draw_time_axis(juce::Graphics& g,
                                       juce::Rectangle<int> area) const {
    g.setColour(juce::Colour::fromRGB(36, 36, 42));
    g.fillRect(area);

    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    const int max_beats = static_cast<int>(area.getWidth() / kPixelsPerBeat) + 1;
    for (int beat = 0; beat <= max_beats; ++beat) {
        const double x = kHeaderWidth + beat * kPixelsPerBeat;
        g.drawText(juce::String(beat + 1),
                   juce::Rectangle<int>{static_cast<int>(x) + 2, area.getY(),
                                        static_cast<int>(kPixelsPerBeat),
                                        area.getHeight()},
                   juce::Justification::centredLeft);
    }

    g.setColour(juce::Colour::fromRGB(60, 60, 72));
    g.drawHorizontalLine(area.getBottom() - 1, static_cast<float>(area.getX()),
                         static_cast<float>(area.getRight()));
}

void SongViewComponent::draw_track_rows(juce::Graphics& g,
                                        juce::Rectangle<int> area) const {
    const auto& project = controller_.project();
    const auto& tracks = project.tracks();

    // Row backgrounds + bar/beat grid.
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        const auto y = area.getY() + static_cast<int>(i * kRowHeight);
        const auto row =
            juce::Rectangle<int>{area.getX(), y, area.getWidth(), kRowHeight};
        g.setColour((i % 2) == 0 ? juce::Colour::fromRGB(32, 32, 38)
                                 : juce::Colour::fromRGB(36, 36, 42));
        g.fillRect(row);
    }

    // Vertical bar / beat lines across all rows.
    const int max_beats = static_cast<int>(area.getWidth() / kPixelsPerBeat) + 1;
    const int total_rows_height = static_cast<int>(tracks.size() * kRowHeight);
    for (int beat = 0; beat <= max_beats; ++beat) {
        const double x = kHeaderWidth + beat * kPixelsPerBeat;
        const bool is_bar = beat % 4 == 0;
        g.setColour(is_bar ? juce::Colour::fromRGB(70, 70, 84)
                           : juce::Colour::fromRGB(45, 45, 54));
        g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()),
                           static_cast<float>(area.getY() + total_rows_height));
    }

    // Pattern placements as coloured blocks.
    for (const auto& pl : project.arrangement()) {
        // Locate row by track id.
        int row_index = -1;
        for (std::size_t i = 0; i < tracks.size(); ++i) {
            if (tracks[i].id == pl.track_id) {
                row_index = static_cast<int>(i);
                break;
            }
        }
        if (row_index < 0) {
            continue;
        }

        const auto* pattern = tracks[static_cast<std::size_t>(row_index)]
                                  .track.find_pattern(pl.pattern_id);
        if (pattern == nullptr) {
            continue;
        }

        const auto x_start = tick_to_x(pl.start);
        const auto x_end = tick_to_x(pl.start + pattern->length());
        const int y = area.getY() + row_index * kRowHeight + 3;
        const auto rect = juce::Rectangle<float>(
            static_cast<float>(x_start), static_cast<float>(y),
            static_cast<float>(std::max(2.0, x_end - x_start)),
            static_cast<float>(kRowHeight - 6));

        const auto colour = pattern_colour(pl.pattern_id);
        g.setColour(colour);
        g.fillRoundedRectangle(rect, 3.0f);
        g.setColour(colour.darker(0.5f));
        g.drawRoundedRectangle(rect, 3.0f, 1.0f);

        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.setFont(juce::Font(juce::FontOptions(10.5f)));
        g.drawText(juce::String(pattern->name()), rect.reduced(4.0f, 1.0f),
                   juce::Justification::centredLeft, true);
    }
}

void SongViewComponent::draw_loop_region(juce::Graphics& g,
                                         juce::Rectangle<int> area) const {
    const auto loop = controller_.project().loop();
    if (!loop.enabled()) {
        return;
    }
    const auto x_start = tick_to_x(loop.start);
    const auto x_end = tick_to_x(loop.end);
    g.setColour(juce::Colour::fromRGB(255, 200, 100).withAlpha(0.10f));
    g.fillRect(juce::Rectangle<float>(
        static_cast<float>(x_start), static_cast<float>(area.getY()),
        static_cast<float>(x_end - x_start), static_cast<float>(area.getHeight())));
    g.setColour(juce::Colour::fromRGB(255, 200, 100).withAlpha(0.7f));
    g.drawVerticalLine(static_cast<int>(x_start), static_cast<float>(area.getY()),
                       static_cast<float>(area.getBottom()));
    g.drawVerticalLine(static_cast<int>(x_end), static_cast<float>(area.getY()),
                       static_cast<float>(area.getBottom()));
}

void SongViewComponent::draw_playhead(juce::Graphics& g,
                                      juce::Rectangle<int> area) const {
    const auto pos = controller_.position();
    const auto x = tick_to_x(pos);
    if (x < area.getX() || x > area.getRight()) {
        return;
    }
    g.setColour(juce::Colours::orangered.withAlpha(0.85f));
    g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()),
                       static_cast<float>(area.getBottom()));
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

void SongViewComponent::mouseDoubleClick(const juce::MouseEvent& e) {
    if (!nav_) {
        return;
    }
    const int row = track_index_at_y(e.y);
    if (row < 0) {
        return;
    }
    const auto& project = controller_.project();
    if (row >= static_cast<int>(project.tracks().size())) {
        return;
    }
    const auto& track_entry = project.tracks()[static_cast<std::size_t>(row)];
    const auto click_tick = tick_at_x(e.x);

    // Find the placement on this track that contains click_tick.
    for (const auto& pl : project.arrangement()) {
        if (pl.track_id != track_entry.id) {
            continue;
        }
        const auto* pattern = track_entry.track.find_pattern(pl.pattern_id);
        if (pattern == nullptr) {
            continue;
        }
        const auto end = pl.start + pattern->length();
        if (click_tick >= pl.start && click_tick < end) {
            nav_(track_entry.id, pl.pattern_id);
            return;
        }
    }
}

}  // namespace turdus::ui
