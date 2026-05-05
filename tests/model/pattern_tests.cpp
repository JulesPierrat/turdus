#include <catch2/catch_test_macros.hpp>

#include <turdus/core/Channel.hpp>
#include <turdus/core/Pitch.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/core/Velocity.hpp>
#include <turdus/model/Note.hpp>
#include <turdus/model/Pattern.hpp>

using namespace turdus::core;
using namespace turdus::model;

namespace {
Note make_note(int pitch, int start, int length, int velocity = 100) {
    return Note{Pitch{pitch}, Tick{start}, Tick{length}, Velocity{velocity}};
}
}  // namespace

TEST_CASE("Pattern default-constructs", "[model][pattern]") {
    Pattern p;
    REQUIRE(p.name() == "Pattern");
    REQUIRE(p.length() == Tick{3840});  // 4 beats * 960 PPQ
    REQUIRE(p.default_channel() == Channel{0});
    REQUIRE(p.empty());
}

TEST_CASE("Pattern setters", "[model][pattern]") {
    Pattern p;
    p.set_name("Verse");
    p.set_length(Tick{1920});
    p.set_default_channel(Channel{3});
    REQUIRE(p.name() == "Verse");
    REQUIRE(p.length() == Tick{1920});
    REQUIRE(p.default_channel() == Channel{3});
}

TEST_CASE("Pattern::add_note keeps notes sorted by start", "[model][pattern]") {
    Pattern p;
    p.add_note(make_note(60, 480, 240));
    p.add_note(make_note(62, 0, 240));
    p.add_note(make_note(64, 240, 240));

    const auto& notes = p.notes();
    REQUIRE(notes.size() == 3);
    REQUIRE(notes[0].note.start == Tick{0});
    REQUIRE(notes[1].note.start == Tick{240});
    REQUIRE(notes[2].note.start == Tick{480});
}

TEST_CASE("Pattern::add_note returns an id usable to find/remove the note", "[model][pattern]") {
    Pattern p;
    auto id = p.add_note(make_note(60, 0, 240));
    REQUIRE(id.is_valid());

    auto found = p.find_note(id);
    REQUIRE(found.has_value());
    REQUIRE(found->pitch == Pitch{60});

    REQUIRE(p.remove_note(id));
    REQUIRE(p.empty());
    REQUIRE_FALSE(p.find_note(id).has_value());
}

TEST_CASE("Pattern::remove_note returns false for unknown id", "[model][pattern]") {
    Pattern p;
    p.add_note(make_note(60, 0, 240));
    auto bogus = NoteId::from_raw(999999);
    REQUIRE_FALSE(p.remove_note(bogus));
    REQUIRE(p.size() == 1);
}

TEST_CASE("Pattern::update_note re-sorts when start changes", "[model][pattern]") {
    Pattern p;
    auto id_a = p.add_note(make_note(60, 0, 240));
    p.add_note(make_note(62, 480, 240));
    p.add_note(make_note(64, 960, 240));

    REQUIRE(p.update_note(id_a, make_note(60, 1200, 240)));

    const auto& notes = p.notes();
    REQUIRE(notes.front().note.start == Tick{480});
    REQUIRE(notes.back().note.start == Tick{1200});
    REQUIRE(notes.back().id == id_a);
}

TEST_CASE("Pattern equality is content-based", "[model][pattern]") {
    Pattern a("X", Tick{960}, Channel{1});
    Pattern b("X", Tick{960}, Channel{1});
    REQUIRE(a == b);
    a.set_name("Y");
    REQUIRE(a != b);
}

TEST_CASE("Pattern is value-copyable (snapshot semantics)", "[model][pattern]") {
    Pattern original;
    auto id = original.add_note(make_note(60, 0, 240));

    Pattern snapshot = original;
    REQUIRE(snapshot == original);

    original.set_name("modified");
    original.remove_note(id);

    REQUIRE(snapshot.name() == "Pattern");
    REQUIRE(snapshot.size() == 1);
    REQUIRE(snapshot != original);
}
