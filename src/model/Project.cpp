#include <turdus/model/Project.hpp>

#include <algorithm>
#include <utility>

namespace turdus::model {

Project::Project() = default;

void Project::set_name(std::string name) { name_ = std::move(name); }
void Project::set_tempo(core::Bpm tempo) noexcept { tempo_ = tempo; }
void Project::set_time_signature(TimeSignature ts) noexcept { time_signature_ = ts; }

TrackId Project::add_track(Track t) {
    TrackEntry entry{TrackId::next(), std::move(t)};
    tracks_.push_back(std::move(entry));
    return tracks_.back().id;
}

bool Project::add_track_with_id(TrackId id, Track t) {
    if (!id.is_valid()) {
        return false;
    }
    if (std::find_if(tracks_.begin(), tracks_.end(),
                     [id](const TrackEntry& e) { return e.id == id; })
        != tracks_.end()) {
        return false;
    }
    tracks_.push_back(TrackEntry{id, std::move(t)});
    return true;
}

bool Project::remove_track(TrackId id) {
    auto it = std::find_if(tracks_.begin(), tracks_.end(),
                           [id](const TrackEntry& e) { return e.id == id; });
    if (it == tracks_.end()) {
        return false;
    }
    tracks_.erase(it);
    arrangement_.erase(std::remove_if(arrangement_.begin(), arrangement_.end(),
                                      [id](const PatternPlacement& p) { return p.track_id == id; }),
                       arrangement_.end());
    return true;
}

Track* Project::find_track(TrackId id) {
    auto it = std::find_if(tracks_.begin(), tracks_.end(),
                           [id](const TrackEntry& e) { return e.id == id; });
    return it == tracks_.end() ? nullptr : &it->track;
}

const Track* Project::find_track(TrackId id) const {
    auto it = std::find_if(tracks_.begin(), tracks_.end(),
                           [id](const TrackEntry& e) { return e.id == id; });
    return it == tracks_.end() ? nullptr : &it->track;
}

void Project::add_placement(PatternPlacement p) { arrangement_.push_back(p); }

bool Project::remove_placement(std::size_t index) {
    if (index >= arrangement_.size()) {
        return false;
    }
    arrangement_.erase(arrangement_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

void Project::clear_arrangement() noexcept { arrangement_.clear(); }

void Project::add_port_mapping(MidiPortMapping m) {
    if (find_port_mapping(m.label) != nullptr) {
        return;  // labels are unique; ignore duplicates
    }
    port_mappings_.push_back(std::move(m));
}

bool Project::remove_port_mapping(const std::string& label) {
    auto it = std::find_if(port_mappings_.begin(), port_mappings_.end(),
                           [&](const MidiPortMapping& m) { return m.label == label; });
    if (it == port_mappings_.end()) {
        return false;
    }
    port_mappings_.erase(it);
    return true;
}

MidiPortMapping* Project::find_port_mapping(const std::string& label) {
    auto it = std::find_if(port_mappings_.begin(), port_mappings_.end(),
                           [&](const MidiPortMapping& m) { return m.label == label; });
    return it == port_mappings_.end() ? nullptr : &*it;
}

const MidiPortMapping* Project::find_port_mapping(const std::string& label) const {
    auto it = std::find_if(port_mappings_.begin(), port_mappings_.end(),
                           [&](const MidiPortMapping& m) { return m.label == label; });
    return it == port_mappings_.end() ? nullptr : &*it;
}

}  // namespace turdus::model
