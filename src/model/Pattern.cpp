#include <turdus/model/Pattern.hpp>

#include <algorithm>
#include <utility>

#include <turdus/core/Beats.hpp>
#include <turdus/core/Ppq.hpp>

namespace turdus::model {

namespace {
auto by_start = [](const Pattern::Entry& a, const Pattern::Entry& b) {
    return a.note.start < b.note.start;
};
}  // namespace

Pattern::Pattern()
    : name_("Pattern"),
      length_(core::beats_to_ticks(core::Beats{4.0})),
      default_channel_(core::Channel{0}) {}

Pattern::Pattern(std::string name, core::Tick length, core::Channel default_channel)
    : name_(std::move(name)), length_(length), default_channel_(default_channel) {}

void Pattern::set_name(std::string name) { name_ = std::move(name); }
void Pattern::set_length(core::Tick length) { length_ = length; }
void Pattern::set_default_channel(core::Channel ch) { default_channel_ = ch; }

NoteId Pattern::add_note(Note note) {
    Entry entry{NoteId::next(), note};
    auto it = std::upper_bound(notes_.begin(), notes_.end(), entry, by_start);
    notes_.insert(it, entry);
    return entry.id;
}

bool Pattern::add_note_with_id(NoteId id, Note note) {
    if (!id.is_valid()) {
        return false;
    }
    if (std::find_if(notes_.begin(), notes_.end(), [id](const Entry& e) { return e.id == id; })
        != notes_.end()) {
        return false;
    }
    Entry entry{id, note};
    auto it = std::upper_bound(notes_.begin(), notes_.end(), entry, by_start);
    notes_.insert(it, entry);
    return true;
}

bool Pattern::remove_note(NoteId id) {
    auto it = std::find_if(notes_.begin(), notes_.end(), [id](const Entry& e) { return e.id == id; });
    if (it == notes_.end()) {
        return false;
    }
    notes_.erase(it);
    return true;
}

bool Pattern::update_note(NoteId id, Note note) {
    auto it = std::find_if(notes_.begin(), notes_.end(), [id](const Entry& e) { return e.id == id; });
    if (it == notes_.end()) {
        return false;
    }
    it->note = note;
    std::stable_sort(notes_.begin(), notes_.end(), by_start);
    return true;
}

std::optional<Note> Pattern::find_note(NoteId id) const {
    auto it = std::find_if(notes_.begin(), notes_.end(), [id](const Entry& e) { return e.id == id; });
    if (it == notes_.end()) {
        return std::nullopt;
    }
    return it->note;
}

}  // namespace turdus::model
