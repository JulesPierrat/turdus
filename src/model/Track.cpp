#include <turdus/model/Track.hpp>

#include <algorithm>
#include <utility>

namespace turdus::model {

Track::Track() : channel_(core::Channel{0}) {}

Track::Track(std::string name, std::string port_label, core::Channel channel)
    : name_(std::move(name)), port_label_(std::move(port_label)), channel_(channel) {}

void Track::set_name(std::string name) { name_ = std::move(name); }
void Track::set_port_label(std::string label) { port_label_ = std::move(label); }
void Track::set_channel(core::Channel ch) { channel_ = ch; }

void Track::set_transpose(int semitones) {
    transpose_ = std::clamp(semitones, kMinTranspose, kMaxTranspose);
}

PatternId Track::add_pattern(Pattern p) {
    PatternEntry entry{PatternId::next(), std::move(p)};
    patterns_.push_back(std::move(entry));
    return patterns_.back().id;
}

bool Track::add_pattern_with_id(PatternId id, Pattern p) {
    if (!id.is_valid()) {
        return false;
    }
    if (std::find_if(patterns_.begin(), patterns_.end(),
                     [id](const PatternEntry& e) { return e.id == id; })
        != patterns_.end()) {
        return false;
    }
    patterns_.push_back(PatternEntry{id, std::move(p)});
    return true;
}

bool Track::remove_pattern(PatternId id) {
    auto it = std::find_if(patterns_.begin(), patterns_.end(),
                           [id](const PatternEntry& e) { return e.id == id; });
    if (it == patterns_.end()) {
        return false;
    }
    patterns_.erase(it);
    return true;
}

Pattern* Track::find_pattern(PatternId id) {
    auto it = std::find_if(patterns_.begin(), patterns_.end(),
                           [id](const PatternEntry& e) { return e.id == id; });
    return it == patterns_.end() ? nullptr : &it->pattern;
}

const Pattern* Track::find_pattern(PatternId id) const {
    auto it = std::find_if(patterns_.begin(), patterns_.end(),
                           [id](const PatternEntry& e) { return e.id == id; });
    return it == patterns_.end() ? nullptr : &it->pattern;
}

}  // namespace turdus::model
