#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <turdus/core/Bpm.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/model/TimeSignature.hpp>
#include <turdus/model/Track.hpp>

namespace turdus::model {

// A scheduled instance of a pattern on a track at a given starting tick.
// Length and looping behavior come from the referenced Pattern (engine-side concern).
struct PatternPlacement {
    TrackId track_id;
    PatternId pattern_id;
    core::Tick start;

    constexpr auto operator<=>(const PatternPlacement&) const noexcept = default;
};

// Maps a project-local port label (used by Track::port_label) to a physical
// MIDI device name. Lets a project move between machines whose port names differ.
struct MidiPortMapping {
    std::string label;
    std::string device_name;
    bool send_clock{false};

    bool operator==(const MidiPortMapping&) const = default;
};

// Song-level loop region in absolute song ticks. Disabled when start == end —
// transport then plays linearly without wrapping.
struct LoopRegion {
    core::Tick start{0};
    core::Tick end{0};

    constexpr bool enabled() const noexcept { return end > start; }
    constexpr core::Tick length() const noexcept { return end - start; }

    constexpr auto operator<=>(const LoopRegion&) const noexcept = default;
};

class Project {
public:
    struct TrackEntry {
        TrackId id;
        Track track;

        bool operator==(const TrackEntry&) const = default;
    };

    Project();

    const std::string& name() const noexcept { return name_; }
    void set_name(std::string name);

    core::Bpm tempo() const noexcept { return tempo_; }
    void set_tempo(core::Bpm tempo) noexcept;

    TimeSignature time_signature() const noexcept { return time_signature_; }
    void set_time_signature(TimeSignature ts) noexcept;

    const std::vector<TrackEntry>& tracks() const noexcept { return tracks_; }
    TrackId add_track(Track t);

    // Insert a track with a caller-supplied id (for deserialization). Returns false
    // if the id is invalid or already present in this project.
    bool add_track_with_id(TrackId id, Track t);

    bool remove_track(TrackId id);
    Track* find_track(TrackId id);
    const Track* find_track(TrackId id) const;

    const std::vector<PatternPlacement>& arrangement() const noexcept { return arrangement_; }
    void add_placement(PatternPlacement p);
    bool remove_placement(std::size_t index);
    void clear_arrangement() noexcept;

    const std::vector<MidiPortMapping>& port_mappings() const noexcept { return port_mappings_; }
    void add_port_mapping(MidiPortMapping m);
    bool remove_port_mapping(const std::string& label);
    MidiPortMapping* find_port_mapping(const std::string& label);
    const MidiPortMapping* find_port_mapping(const std::string& label) const;

    LoopRegion loop() const noexcept { return loop_; }
    void set_loop(LoopRegion loop) noexcept { loop_ = loop; }

    bool operator==(const Project&) const = default;

private:
    std::string name_;
    core::Bpm tempo_;
    TimeSignature time_signature_;
    std::vector<TrackEntry> tracks_;
    std::vector<PatternPlacement> arrangement_;
    std::vector<MidiPortMapping> port_mappings_;
    LoopRegion loop_{};
};

}  // namespace turdus::model
