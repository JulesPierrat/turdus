#pragma once

#include <optional>
#include <string>
#include <vector>

#include <turdus/core/Channel.hpp>
#include <turdus/core/Id.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/model/Note.hpp>

namespace turdus::model {

struct NoteTag;
using NoteId = core::Id<NoteTag>;

struct PatternTag;
using PatternId = core::Id<PatternTag>;

class Pattern {
public:
    struct Entry {
        NoteId id;
        Note note;

        constexpr auto operator<=>(const Entry&) const noexcept = default;
    };

    Pattern();
    Pattern(std::string name, core::Tick length, core::Channel default_channel);

    const std::string& name() const noexcept { return name_; }
    void set_name(std::string name);

    core::Tick length() const noexcept { return length_; }
    void set_length(core::Tick length);

    core::Channel default_channel() const noexcept { return default_channel_; }
    void set_default_channel(core::Channel ch);

    const std::vector<Entry>& notes() const noexcept { return notes_; }
    std::size_t size() const noexcept { return notes_.size(); }
    bool empty() const noexcept { return notes_.empty(); }

    NoteId add_note(Note note);

    // Insert a note with a caller-supplied id (for deserialization). Returns false if
    // the id is invalid or already present in this pattern.
    bool add_note_with_id(NoteId id, Note note);

    bool remove_note(NoteId id);
    bool update_note(NoteId id, Note note);
    std::optional<Note> find_note(NoteId id) const;

    bool operator==(const Pattern&) const = default;

private:
    std::string name_;
    core::Tick length_;
    core::Channel default_channel_;
    std::vector<Entry> notes_;  // sorted ascending by note.start
};

}  // namespace turdus::model
